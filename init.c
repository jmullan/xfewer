/*
 * Copyright (C) 1994 by Dave Glowacki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * to rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVE GLOWACKI BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * $Header: /usr/sww/share/src/X11R6/local/applications/xfewer-1.7/RCS/init.c,v 1.45 1994/07/29 03:07:33 dglo Exp $
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/X.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>

#include "xfewer.h"

#define FontWidth(fnt)		((fnt)->max_bounds.width)
#define FontHeight(fnt)		((fnt)->ascent + (fnt)->descent)

static void fixNewText __P((char *, unsigned *, unsigned *, unsigned *));
static void restartNewInputHandler __P((XtPointer, XtIntervalId *));
static void handleNewInput __P((XtPointer, int *, XtInputId *));
static Widget makeButton __P((Widget, int, unsigned, Widget,
			      XtCallbackProc, WindowInfo *, String,
			      const char *));
static void ParseGeometryRsrc __P((void));

#define DEFAULT_WIDTH	80
#define DEFAULT_HEIGHT	24

static int geomParsed = 0;
static int geomMask, geomX, geomY;
static unsigned int geomWidth, geomHeight;
static char *geomPosition = NULL;

static void
fixNewText(text, lp, wp, hp)
     char *text;
     unsigned *lp, *wp, *hp;
{
    char *tmp;
    int i, w;

    /* lose all underlined and bold characters in the file */
    i = w = 0;
    tmp = text;
    while (i < *lp) {
        if ((i+1 < *lp) && (*(text + i) == '_') &&
            (*(text + i + 1) == '\b')) {
            i += 2;
        } else if ((i+2 < *lp) && (*(text + i) == *(text + i + 2)) &&
                   (*(text + i + 1) == '\b')) {
            i += 2;
        } else {
            *tmp = *(text + i++);
            if (*tmp == '\n') {
                if (w > *wp)
                    *wp = w;
                w = 0;
                (*hp)++;
            } else if (*tmp == '\t') {
                w += 8 - (w % 8);
            } else
                w++;
            tmp++;
        }
    }
    *tmp++ = 0;
    *lp = (tmp - text);
}

static void
restartNewInputHandler(closure, ignored)
     XtPointer closure;
     XtIntervalId *ignored;
{
    WindowInfo *wi = (WindowInfo *)closure;

    wi->inputInfo->id = XtAppAddInput(context, wi->inputInfo->fd,
                                      (XtPointer )XtInputReadMask,
                                      handleNewInput, (XtPointer )wi);
}

static void
handleNewInput(closure, source, id)
     XtPointer closure;
     int *source;
     XtInputId *id;
{
    WindowInfo *wi = (WindowInfo *)closure;
    char buffer[BUFSIZ];
    unsigned nbytes, lastchar;
    Cardinal needed;
    char *bottom;
    XawTextPosition pos;

    if ((nbytes = read(*source, buffer, (size_t )BUFSIZ)) == -1) {
        XtAppError(context, "handleNewMessage: read failed");
        return;
    }

    if (nbytes > 0) {
        needed = wi->used + nbytes + (wi->flag & XFewerAddedNewline ? 0 : 1);
        if (needed > wi->allocated) {
            wi->memory = XtRealloc((char *)wi->memory, needed);
            wi->allocated = needed;
        }
        lastchar = wi->used - (wi->flag & XFewerAddedNewline ? 2 : 1);
        bottom = (char *)&(wi->memory[lastchar]);
        strncpy(bottom, buffer, (size_t )nbytes);
        wi->flag = (wi->flag & ~(unsigned )XFewerAddedNewline);
        if (bottom[nbytes-1] != '\n') {
            wi->flag |= XFewerAddedNewline;
            bottom[nbytes++] = '\n';
        }
        bottom[nbytes] = 0;
        fixNewText(bottom, &nbytes, &(wi->dataWidth), &(wi->dataHeight));
        wi->used = lastchar + nbytes;

        pos = XawTextGetInsertionPoint(wi->text);
        XtVaSetValues(wi->text,
                      XtNstring, wi->memory,
                      NULL);
        XawTextSetInsertionPoint(wi->text,
                                 (pos != lastchar ? pos : wi->used -
                                  (wi->flag & XFewerAddedNewline ? 2 : 1)));
    } else {

        /* no input was waiting for us */
        if (wi->inputInfo->falseAlarm++ >= 3) {

            /* bump up the wait interval */
            wi->inputInfo->interval <<= 1;
            wi->inputInfo->falseAlarm = 0;
        }

        /* turn off input source... */
        XtRemoveInput(wi->inputInfo->id);

        /* ...but set alarm to turn it back on after the appropriate interval */
        XtAppAddTimeOut(context, wi->inputInfo->interval, restartNewInputHandler,
                        (XtPointer )wi);
    }
}

static char *
readFile(fd, wi, szp)
     int fd;
     WindowInfo *wi;
     Cardinal *szp;
{
    struct stat fileinfo;	/* file information from fstat */
    char *data;
    XtInputId id;

    /* get size of file */
    if (fstat(fd, &fileinfo)) {
        fprintf(stderr, "%s: can't get file size.\n", progname);
        exit(1);
    }

    /* leave space for the NULL (and possibly a final NEWLINE) */
    data = (char *)XtMalloc((Cardinal )(fileinfo.st_size + 2));

    /* read entire file */
    ssize_t r = read(fd, data, (size_t )fileinfo.st_size);
    if (-1 == r) {
        fprintf(stderr, "%s: can't read from file.\n", progname);
        close(fd);
        exit(1);
    } else if (r != (size_t )fileinfo.st_size) {
        fprintf(stderr, "%s: did not read expected amount from file.\n", progname);
    }
    if (resources.monitorFile && wi) {
        id = XtAppAddInput(context, fd, (XtPointer )XtInputReadMask,
                           handleNewInput, (XtPointer )wi);
        if (wi) {
            wi->inputInfo = (InputInfo *)XtMalloc((Cardinal )sizeof(InputInfo));
            wi->inputInfo->fd = fd;
            wi->inputInfo->id = id;
            wi->inputInfo->interval = 1000;
            wi->inputInfo->falseAlarm = 0;
        }

    } else {
        close(fd);

        /* make sure file ends with a NEWLINE */
        if (*(data + fileinfo.st_size - 1) != '\n') {
            *(data + fileinfo.st_size) = '\n';
            fileinfo.st_size++;
            if (wi)
                wi->flag |= XFewerAddedNewline;
        }
    }

    *szp = fileinfo.st_size;
    return(data);
}

static char *
readStdIn(fd, wi, szp)
     int fd;
     WindowInfo *wi;
     Cardinal *szp;
{
    XtInputId id;
    char *data, *where;
    int size, sz, newsz;

    if (resources.monitorFile && wi) {

        /* only allocate a single byte, read everything dynamically */
        data = (char *)XtMalloc(2);
        size = 1;
        *data = '\n';
        if (wi) {
            wi->flag |= XFewerAddedNewline;
            id = XtAppAddInput(context, 0, (XtPointer )XtInputReadMask,
                               handleNewInput, (XtPointer )wi);
            wi->inputInfo = (InputInfo *)XtMalloc((Cardinal )sizeof(InputInfo));
            wi->inputInfo->fd = 0;
            wi->inputInfo->id = id;
            wi->inputInfo->interval = 1000;
            wi->inputInfo->falseAlarm = 0;
        }

    } else {

        /* read STDIN a chunk at a time */
        data = (char *)XtMalloc(XFEWER_MEMORY_CHUNK);
        size = 0;
        where = data;
        while ((sz = read(fd, where, (size_t )XFEWER_MEMORY_CHUNK)) != 0) {
            size += sz;
            newsz = size + XFEWER_MEMORY_CHUNK;
            data = (char *)XtRealloc(data, (unsigned)newsz);
            where = data + size;
        }

        /* see if file ends with a NEWLINE */
        if (*(data + size - 1) != '\n')
            size++;

        /* allocate final chunk of memory */
        newsz = size + 1;
        data = (char *)XtRealloc(data, (unsigned)newsz);

        /* make sure file ends with a NEWLINE */
        if (*(data + size - 1) != '\n')
            *(data + size - 1) = '\n';

    }

    *szp = size;
    return(data);
}

const char *
InitData(fd, wi)
     int fd;
     WindowInfo *wi;
{
    char *data;		/* pointer to the data stored
			   in dynamic memory */
    unsigned width, height, length;
    Cardinal size;

    /*
     * Get file size and allocate a chunk of memory for the file to be
     * copied into.
     */

    if (fd != 0) {
        data = readFile(fd, wi, &size);
    } else {
        data = readStdIn(fd, wi, &size);
    }
    /* NUL-terminate file string */
    *(data + size) = '\0';

    width = height = 0;
    length = size;
    fixNewText((char *)data, &length, &width, &height);

    if (wi) {
        wi->memory = data;
        wi->allocated = size + 1;
        wi->used = length;
        wi->dataWidth = width;
        wi->dataHeight = height;
    }

    return(data);
}

static Widget
makeButton(parent, sensitive, vertdist, prevwidg, procptr, wininfo,
	   accelstr, label)
     Widget parent;
     int sensitive;
     unsigned vertdist;
     Widget prevwidg;
     XtCallbackProc procptr;
     WindowInfo *wininfo;
     String accelstr;
     const char *label;
{
    XtAccelerators accel;
    XtCallbackRec callback[2];
    Widget w;

    accel = XtParseAcceleratorTable(accelstr);

    callback[0].callback = procptr;
    callback[0].closure = (XtPointer )wininfo;
    callback[1].callback = (XtCallbackProc) NULL;
    callback[1].closure = (XtPointer) NULL;

    w = XtVaCreateManagedWidget(label, commandWidgetClass, parent,
                                XtNaccelerators, accel,
                                XtNbottom, XtChainTop,
                                XtNcallback, callback,
                                XtNcursor, stdcur,
                                XtNfont, buttonFont,
                                XtNfromVert, prevwidg,
                                XtNfromHoriz, NULL,
                                XtNhorizDistance, 4,
                                XtNleft, XtChainLeft,
                                XtNright, XtChainLeft,
                                XtNsensitive, (sensitive ? False : True),
                                XtNtop, XtChainTop,
                                XtNvertDistance, vertdist,
                                XtNwidth, XFEWER_BUTTON_WIDTH,
                                NULL);

    XtInstallAccelerators(wininfo->text, w);
    return(w);
}

/*
 *	Function Name: MakeToolbox
 *	Description:   This function create all the widgets necessary
 *		       to build the toolbox.
 *	Arguments:     parent   - the parent widget
 *		       cd       - pointer to the data structure containing
 *			          the application data.
 *		       notmain  - TRUE if this is not the main window
 *		       new_wdg  - the shell widget that is the new window
 *		       filename - the name of the file displayed in the window
 *	Returns:       None.
 */

Widget
MakeToolbox(parent, wi, filename)
     Widget parent;
     WindowInfo *wi;
     const char *filename;
{
    Widget box, button;
    const String AccSearch = "#override Meta<Key>S:   set() notify() unset()\n";
    const String AccNext = "#override Meta<Key>N:   set() notify() unset()\n";
    const String AccReload = "#override Meta<Key>R:   set() notify() unset()\n";
    const String AccEdit = "#override Meta<Key>E:   set() notify() unset()\n";
    const String AccChange = "#override Meta<Key>C:   set() notify() unset()\n";
    const String AccWind = "#override Meta<Key>W:   set() notify() unset()\n";
    const String AccPrint = "#override Meta<Key>P: set() notify() unset()\n";
    const String AccHelp = "#override <Key>?: set() notify() unset()\n";
    const String AccClose = "#override <Key>Q: set() notify() unset()\n";
    const String AccBackgnd = "#override Meta<Key>&: set() notify() unset()\n";
    const String AccQuit = "#override Meta<Key>Q: set() notify() unset()\n";

    /* create the ButtonBox which contains the toolbox */
    box = XtVaCreateManagedWidget("toolbox", boxWidgetClass, parent,
                                  XtNallowResize, True,
                                  XtNbottom, XtChainTop,
                                  XtNfromHoriz, wi->text,
                                  XtNhorizDistance, 1,
                                  XtNleft, XtChainRight,
                                  XtNorientation, XtorientVertical,
                                  XtNright, XtChainRight,
                                  XtNshowGrip, False,
                                  XtNskipAdjust, True,
                                  XtNtop, XtChainTop,
                                  NULL);

    button = (Widget )NULL;
    button = makeButton(box, False, 1, button, PopupHelp, wi, AccHelp, "Help");
    button = makeButton(box, False, 1, button, Search, wi, AccSearch, "Search");
    button = makeButton(box, False, 1, button, SearchNext, wi,
                        AccNext, "Search Next");
    button = makeButton(box, (filename == NULL ? True : False), 1, button,
                        CallEditor, wi, AccEdit, "Editor");
    if (filename == NULL)
        wi->editorButton = button;
    else
        wi->editorButton = 0;
    button = makeButton(box, ((filename == NULL || wi->inputInfo != NULL) ?
                              True : False),
                        1, button, Reload, wi, AccReload, "Reload");
    if (filename == NULL)
        wi->reloadButton = button;
    else
        wi->reloadButton = 0;
    button = makeButton(box, False, 1, button, ChangeFile, wi,
                        AccChange, "Change file");
    button = makeButton(box, False, 1, button, NewWindow, wi,
                        AccWind, "New window");
    button = makeButton(box, False, 1, button, Print, wi, AccPrint, "Print");
    button = makeButton(box, False, 1, button, Background, wi, AccBackgnd,
                        "Background");
    button = makeButton(box, False, 1, button, CloseWindow, wi, AccClose,
                        "Close window");
    if (resources.quitButton)
        button = makeButton(box, False, 1, button, Quit, wi, AccQuit, "Quit");

    return(box);
}

static void
ParseGeometryRsrc()
{
    /* get geometry from resource */
    if (resources.geometry) {
        geomMask = XParseGeometry(resources.geometry, &geomX, &geomY,
                                  &geomWidth, &geomHeight);

        /* supply width & height if unspecified */
        if ((geomMask & WidthValue) == 0 || (geomMask & HeightValue) == 0) {
            if (((geomMask & WidthValue) && geomWidth <= 0) ||
                ((geomMask & HeightValue) && geomHeight <= 0))
                fprintf(stderr, "Bad WIDTHxHEIGHT in geometry \"%s\"!\n",
                        resources.geometry);
            geomWidth = DEFAULT_WIDTH;
            geomHeight = DEFAULT_HEIGHT;
        }
    } else {
        geomWidth = DEFAULT_WIDTH;
        geomHeight = DEFAULT_HEIGHT;
    }

    /* remember that we've been here */
    geomParsed = 1;
}

const char *
GetGeometryPosition()
{
    if (!geomParsed)
        ParseGeometryRsrc();

    if (!geomPosition && (geomMask & XValue) && (geomMask & YValue)) {
        geomPosition = XtMalloc(32);
        sprintf(geomPosition, "%+d%+d", geomX, geomY);
    }

    return(geomPosition);
}

/*
 *	Function Name: MakeText
 *	Description:   This function creates the text widget necessary
 *		       to display the data.
 *	Arguments:     parent     - the parent widget
 *		       wi         - pointer to window information
 *		       data       - pointer to data in memory.
 *		       ignoreGeom - TRUE if we should ignore supplied geometry
 *	Returns:       tmp        - the text widget so created.
 */

Widget
MakeText(parent, wi, data, ignoreGeom)
     Widget parent;
     WindowInfo *wi;
     const char *data;
     int ignoreGeom;
{
    int windowWidth, windowHeight;
    int charWidth, charHeight;
    Widget txt;

    /* make sure we have default width and height */
    if (!geomParsed)
        ParseGeometryRsrc();

    /* set window width & height (expressed in characters) */
    windowWidth = (ignoreGeom ? DEFAULT_WIDTH : geomWidth);
    windowHeight = (ignoreGeom ? DEFAULT_HEIGHT : geomHeight);

    /* get width/height of a text window character */
    charWidth = FontWidth(textFont);
    charHeight = FontHeight(textFont);

    /* if actual text is smaller, use it instead */
    if (wi && (wi->inputInfo == NULL) && resources.sizeToFit && (resources.geometry == NULL)) {
        if (windowWidth > wi->dataWidth) {
            windowWidth = wi->dataWidth + 1;
        }
        if (windowHeight > wi->dataHeight) {
            windowHeight = wi->dataHeight;
        }
    }

    /* convert width/height of text window to pixel values */
    windowWidth *= charWidth;
    windowHeight *= charHeight;

#ifdef DUMP_TEXTFONT_INFO

    printf("\nTextFont info:\n");
    printf("\tfid=%ld, direction=%d\n", textFont->fid, textFont->direction);
    printf("\tmin_char_or_byte2=%d, max_char_or_byte2=%d\n",
           textFont->min_char_or_byte2, textFont->max_char_or_byte2);
    printf("\tmin_byte1=%d, max_byte1=%d\n", textFont->min_byte1,
           textFont->max_byte1);
    printf("\tall_chars_exist=%s, default_char=%d, n_properties=%d\n",
           (textFont->all_chars_exist ? "TRUE" : "FALSE"),
           textFont->default_char, textFont->n_properties);
    printf("\tmin_bounds: lbearing=%d, rbearing=%d, width=%d,\n",
           textFont->min_bounds.lbearing, textFont->min_bounds.rbearing,
           textFont->min_bounds.width);
    printf("\t\tascent=%d, descent=%d\n", textFont->min_bounds.ascent,
           textFont->min_bounds.descent);
    printf("\tmax_bounds: lbearing=%d, rbearing=%d, width=%d,\n",
           textFont->max_bounds.lbearing, textFont->max_bounds.rbearing,
           textFont->max_bounds.width);
    printf("\t\tascent=%d, descent=%d\n", textFont->max_bounds.ascent,
           textFont->max_bounds.descent);
    printf("\tascent=%d, descent=%d\n", textFont->ascent, textFont->descent);
    printf("TextWindow: char=(%d,%d), lines=(%d,%d), txtwin=(%d,%d)\n",
           charWidth, charHeight,
           (wi ? wi->dataWidth : -1), (wi? wi->dataHeight : -1),
           windowWidth, windowHeight);

#endif /* DUMP_TEXTFONT_INFO */

    txt = XtVaCreateManagedWidget("text", asciiTextWidgetClass, parent,
                                  XtNallowResize, True,
                                  XtNbottom, XtChainBottom,
                                  XtNdisplayNonprinting, False,
                                  XtNeditType, XawtextRead,
                                  XtNfont, textFont,
                                  XtNfromHoriz, NULL,
                                  XtNheight, windowHeight,
                                  XtNhorizDistance, 1,
                                  XtNleft, XtChainLeft,
                                  XtNright, XtChainRight,
                                  XtNskipAdjust, False,
                                  XtNscrollVertical, XawtextScrollWhenNeeded,
                                  XtNscrollHorizontal, XawtextScrollWhenNeeded,
                                  XtNscrollVertical, XawtextScrollAlways,
                                  XtNscrollHorizontal, XawtextScrollAlways,
                                  XtNshowGrip, False,
                                  XtNstring, data,
                                  XtNtop, XtChainTop,
                                  XtNuseStringInPlace, True,
                                  XtNwidth, windowWidth,
                                  NULL);

    return (txt);
}

void
SetXNames(top, filename)
     Widget top;
     const char *filename;
{
    static char *namePrefix;
    static int namelen = 0;
    static char *namestr = 0;
    const char *cp;
    int prefixlen, newlen;

    /* don't do this if user has already named the window */
    if (resources.name && resources.title)
        return;

    /* make sure there's a filename string */
    if (!filename || *filename == 0)
        return;

    /* if there's no namePrefix, set one up */
    if (!namePrefix) {
        namePrefix = XtMalloc((Cardinal )(strlen(progname) + 2));
        strcpy(namePrefix, progname);
        strcat(namePrefix, ":");
    }

    /* figure out how long the prefix is */
    if (namePrefix && *namePrefix)
        prefixlen = strlen(namePrefix);
    else
        prefixlen = 0;

    /* remove path junk if user doesn't want it */
    if (resources.removePath) {
        cp = strrchr(filename, '/');
        if (cp)
            filename = cp + 1;
    }

    /* make sure name buffer is large enough */
    newlen = prefixlen + strlen(filename) + 1;
    if (newlen > namelen) {
        if (namestr) {
            XtFree(namestr);
        }
        namestr = (char *)XtMalloc((Cardinal )newlen);
        namelen = newlen;
    }

    /* create title/icon name string */
    strcpy(namestr, namePrefix);
    strcat(namestr, filename);

    /* set window's icon and title names to the file name */
    XtVaSetValues(top,
                  XtNiconName, (resources.name ? resources.name : namestr),
                  XtNtitle, (resources.title ? resources.title : namestr),
                  NULL);
}

void
SetWMHints(wi)
     WindowInfo *wi;
{
    Dimension txtWidth, txtHeight;
    Dimension winWidth, winHeight;
    Dimension baseWidth, baseHeight;
    Dimension tbxWidth, tbxHeight;
    int charWidth, charHeight;

    /* get width/height of entire window */
    XtVaGetValues(wi->base,
                  XtNheight, &winHeight,
                  XtNwidth, &winWidth,
                  NULL);

    /* get width/height of text window */
    XtVaGetValues(wi->text,
                  XtNheight, &txtHeight,
                  XtNwidth, &txtWidth,
                  NULL);

    /* get size of tool box */
    XtVaGetValues(wi->toolbox,
                  XtNheight, &tbxHeight,
                  XtNwidth, &tbxWidth,
                  NULL);

    /* figure out base width/height */
    baseWidth = winWidth - txtWidth;
    baseHeight = (tbxHeight > txtHeight ? tbxHeight : txtHeight);

    /* get width/height of a text window character */
    charWidth = FontWidth(textFont);
    charHeight = FontHeight(textFont);

    /* set base & increment size */
    XtVaSetValues(wi->base,
                  XtNbaseWidth, baseWidth,
                  XtNbaseHeight, baseHeight,
                  XtNheightInc, charHeight,
                  XtNwidthInc, charWidth,
                  NULL);

    /* set minimum window size */
    XtVaSetValues(wi->base,
                  XtNminHeight, tbxHeight,
                  XtNminWidth, tbxWidth + charWidth + charWidth,
                  NULL);
}
