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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xfewer-1.7/RCS/main.c,v 1.36 1994/07/29 02:55:50 dglo Exp $
 */
#include <stdlib.h>
#include <stdio.h>

#include <X11/X.h>
#include <X11/Xos.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xmu/Xmu.h>

#include "xfewer.h"
#include "version.h"
#include "XFewerTop.icn"

#define XtRSearchType	"SearchType"

XtAppContext context;
Widget toplevel;
Display *disp;
const char *className;
const char *progname;

XFewerResources resources;

static XtResource privResources[] = {
    {XtNgeometry, XtCGeometry, XtRString, sizeof(String),
     XtOffset(XFewerResources *, geometry), XtRString, NULL},
    {XtNname, "Name", XtRString, sizeof(String),
     XtOffset(XFewerResources *, name), XtRString, NULL},
    {XtNtitle, XtCTitle, XtRString, sizeof(String),
     XtOffset(XFewerResources *, title), XtRString, NULL},
    {"helpFile", XtCFile, XtRString, sizeof(String),
     XtOffset(XFewerResources *, helpFile), XtRString, HELPFILE},
    {"standardCur", XtCCursor, XtRCursor, sizeof(Cursor),
     XtOffset(XFewerResources *, cursors.top), XtRString, STANDARDCUR},
    {"dialogCur", XtCCursor, XtRCursor, sizeof(Cursor),
     XtOffset(XFewerResources *, cursors.dialog), XtRString, STANDARDCUR},
    {"standardFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XFewerResources *, fonts.standard), XtRString, STANDARDFONT},
    {"textFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XFewerResources *, fonts.text), XtRString, TEXTFONT},
    {"labelFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XFewerResources *, fonts.label), XtRString, LABELFONT},
    {"buttonFont", XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     XtOffset(XFewerResources *, fonts.button), XtRString, BUTTONFONT},
    {"editor", "Editor", XtRString, sizeof(String),
     XtOffset(XFewerResources *, editor), XtRString, DEFEDITOR},
    {"editorDoesWindows", "EditorDoesWindows", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, editorDoesWindows), XtRString, "False"},
    {"printCmd", "PrintCommand", XtRString, sizeof(String),
     XtOffset(XFewerResources *, oldPrintCmd), XtRString, NULL},
    {"printCommand", "PrintCommand", XtRString, sizeof(String),
     XtOffset(XFewerResources *, printCmd), XtRString, PRINTCMD},
    {"maxWindows", "MaxWindows", XtRInt, sizeof(int),
     XtOffset(XFewerResources *, maxWindows), XtRString, "0"},
    {"quitButton", "QuitButton", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, quitButton), XtRString, "False"},
    {"sizeToFit", "SizeToFit", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, sizeToFit), XtRString, "True"},
    {"removePath", "RemovePath", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, removePath), XtRString, "True"},
    {"helpMessage", "HelpMessage", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, helpMessage), XtRString, "False"},
    {"defaultSearchType", "SearchType", XtRSearchType, sizeof(unsigned),
     XtOffset(XFewerResources *, defaultSearchType), XtRString, "ExactMatch"},
    {"monitorFile", "MonitorFile", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, monitorFile), XtRString, "False"},
    {"printVersion", "PrintVersion", XtRBoolean, sizeof(Boolean),
     XtOffset(XFewerResources *, printVersion), XtRString, "False"},
};

static XrmOptionDescRec options[] = {
    {"-f",	"*monitorFile", XrmoptionNoArg,	(XtPointer )"True"},
    {"-follow",	"*monitorFile", XrmoptionNoArg,	(XtPointer )"True"},
    {"-fn", 	"*textFont", XrmoptionSepArg,	(XtPointer )NULL},
    {"-font", 	"*textFont", XrmoptionSepArg,	(XtPointer )NULL},
    {"-help",	"*helpMessage", XrmoptionNoArg,	(XtPointer )"True"},
    {"-version",	"*printVersion", XrmoptionNoArg, (XtPointer )"True"},
};

static Boolean cvtStringToSearchType __P((Display *, XrmValue *, Cardinal *,
					  XrmValue *, XrmValue *, XtPointer *));
static void cleanup __P((void));
static void argPrint __P((const char *));

#define done(type, value)				\
    {                                                   \
        if (toVal->addr != NULL) {			\
	    if (toVal->size < sizeof(type)) {		\
                toVal->size = sizeof(type);		\
                return False;				\
	    }						\
	    *(type *)(toVal->addr) = (value);		\
        } else {					\
	    static type newValue;			\
							\
	    newValue = (value);				\
	    toVal->addr = (XtPointer )&newValue;	\
        }						\
        toVal->size = sizeof(type);			\
        return True;					\
    }

static Boolean
cvtStringToSearchType(display, args, numArgs, fromVal, toVal, destructorData)
     Display *display;
     XrmValue *args;
     Cardinal *numArgs;
     XrmValue *fromVal;
     XrmValue *toVal;
     XtPointer *destructorData;
{
    static int initialized = 0;
    static XrmQuark QExactMatch, QCaseInsensitive, QRegularExpression;
    char *tmp;
    XrmQuark q;

    if (!initialized) {
        QExactMatch = XrmStringToQuark("exactmatch");
        QCaseInsensitive = XrmStringToQuark("caseinsensitive");
        QRegularExpression = XrmStringToQuark("regularexpression");
        initialized = 1;
    }

    if (*numArgs != 0)
        XtWarning("String to SearchType conversion needs no extra arguments");

    /* get quark for string */
    tmp = XtMalloc((Cardinal )(strlen((char *)fromVal->addr) + 1));
    XmuCopyISOLatin1Lowered(tmp, (char *)fromVal->addr);
    q = XrmStringToQuark(tmp);
    XtFree(tmp);

    if (q == QExactMatch)
        done(unsigned, XFewerClearFlag);

    if (q == QCaseInsensitive)
        done(unsigned, XFewerSearchInsensitive);

    if (q == QRegularExpression)
        done(unsigned, XFewerSearchRegExpr);

    XtDisplayStringConversionWarning(display, fromVal->addr, "XtRSearchType");
    return False;
}

static void
cleanup()
{
    XtDestroyWidget(toplevel);
    XtDestroyApplicationContext(context);
}

static void
argPrint(str)
     const char *str;
{
    static int lineLen = 0;
    static int current = 0;
    int len;

    /* find line length */
    if (lineLen == 0) {
        lineLen = 80;
        current = lineLen;
    }

    len = strlen(str);
    current -= len;
    if (current > 0) {
        fputs(str, stderr);
    } else {
        fprintf(stderr, "\n\t%s", str);
        current = lineLen - (8 + len);
    }
}

int
main(argc, argv)
     int argc;
     char *argv[];
{
    int i, attempted;

#ifdef _DEBUG_MALLOC_INC
    {
        union dbmalloptarg	moa;

        moa.i = 0;
        dbmallopt(MALLOC_CKCHAIN, &moa);
    }
#endif

    /* save program name */
    if ((progname = strrchr(argv[0], '/'))) {
        progname++;
    } else {
        progname = argv[0];
    }
    toplevel = XtVaAppInitialize(&context, XFEWER_CLASS,
                                 options, XtNumber(options),
                                 &argc, argv,
                                 NULL, NULL);

    XtAppSetTypeConverter(context, XtRString, XtRSearchType,
                          cvtStringToSearchType, (XtConvertArgList )NULL,
                          0, XtCacheAll, NULL);

    XtGetApplicationResources(toplevel, (XtPointer )&resources, privResources,
                              XtNumber(privResources), NULL, (Cardinal) 0);

    /* print our version number if user wants it */
    if (resources.printVersion)
        printf("XFewer version %s\n", VERSION);

    /* complain about old printCmd resource usage */
    if (resources.oldPrintCmd != NULL) {
        if (strcmp(resources.printCmd, PRINTCMD) != 0)
            fprintf(stderr, "%s: Both 'printCommand' and 'printCmd' specified!\n",
                    progname);
        else {
            fprintf(stderr, "%s: Please use the 'printCommand' resource", progname);
            fprintf(stderr, " instead of 'printCmd'\n");
            resources.printCmd = resources.oldPrintCmd;
        }
    }

    /* save class name */
    className = XFEWER_CLASS;

    XtAppAddActions(context, actions, numactions);

    disp = XtDisplay(toplevel);

    XtVaSetValues(toplevel,
                  XtNiconPixmap, XCreateBitmapFromData(disp,
                                                       XRootWindow(disp, 0),
                                                       XFewerTop_bits,
                                                       XFewerTop_width,
                                                       XFewerTop_height),
                  NULL);

    CheckFonts();

    attempted = 0;
    for (i = 1; resources.helpMessage || i < argc; i++) {

        /* whine if there's still an argument */
        if (resources.helpMessage || *argv[i] == '-') {
            argPrint("Usage: ");
            argPrint(progname);
            argPrint(" [-f]");
            argPrint(" [-follow]");
            argPrint(" [-fn textFont]");
            argPrint(" [-font textFont]");
            argPrint(" [-help]");
            argPrint(" [-version]");
            argPrint(" [file ...]");
            argPrint("\n");
            argPrint("\t(");
            argPrint(progname);
            argPrint(" also takes input");
            argPrint(" from stdin)\n");
            cleanup();
            exit(0);
        }

        CreateWindow(toplevel, argv[i]);
        attempted++;
    }

    /* die if everything failed */
    if (attempted > 0 && windowcount == 0) {
        fprintf(stderr, "%s: no windows found\n", progname);
        cleanup();
        exit(1);
    }

    /* if we haven't opened a window yet... */
    if (windowcount == 0) {
        /*
         * Not really necessary to call this an error,
         * but if the control terminal (for commands)
         * and the input file (for data) are the same,
         * we get weird results at best.
         */
        if (isatty(fileno(stdin))) {
            fprintf(stderr, "%s: can't take input from terminal\n", progname);
            cleanup();
            exit(1);
        }

        /* bring up controlling window using stdin as input */
        CreateWindow(toplevel, NULL);
    }

    XtAppMainLoop(context);
    return 0;
}
