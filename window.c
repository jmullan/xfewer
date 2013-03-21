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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xless-1.7/RCS/window.c,v 1.24 1994/07/29 02:34:16 dglo Exp $
 */

#include <stdio.h>
#include <fcntl.h>

#include <X11/X.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/Paned.h>

#include <X11/Xmu/Converters.h>

#include "xless.h"
#include "XLessWin.icn"

/* list of all xless windows */
WindowInfo *windowlist = 0;
int windowcount = 0;

/* flag for "too many windows" popup */
static int tooManyFlag = 0;

static void popdownTooMany __P((Widget, XtPointer, XtPointer));

WindowInfo *
createWindowInfo()
{
  WindowInfo *wi;

  wi = (WindowInfo *)XtMalloc((Cardinal )sizeof(WindowInfo));
  wi->base = wi->text = wi->toolbox = NULL;
  wi->searchPopup = wi->newWindowPopup = wi->changeFilePopup = NULL;
  wi->editorButton = wi->reloadButton = NULL;
  wi->memory = NULL;
  wi->allocated = 0;
  wi->used = 0;
  wi->file = NULL;
  wi->flag = XLessClearFlag;
  wi->dataHeight = wi->dataWidth = 0;
  wi->searchBuf = wi->newWindowBuf = wi->changeFileBuf = NULL;
  wi->inputInfo = NULL;
  return(wi);
}

WindowInfo *
findWindowInfo(w)
Widget w;
{
  WindowInfo *wi = windowlist;

  while (w) {

    /* if this is the shell widget... */
    if (XtClass(w) == applicationShellWidgetClass) {

      /* find this window in the list */
      while (wi && (wi->base != w))
        wi = wi->next;
      return(wi);
    }

    /* see if parent is the shell widget */
    w = XtParent(w);
  }

  /* couldn't find a window associated with this widget */
  return(0);
}

static void
popdownTooMany(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown((Widget)closure);
  tooManyFlag = 0;
}

int
CreateWindow(top, filename)
Widget top;
const char *filename;
{
  int fd;
  const char *shellName;
  const char *geom;
  Widget base, mainFrame;
  WindowInfo *wi;
  XLessFlag flag = XLessClearFlag;
  const char *fixedName;
  static Widget badFileMsg = 0;

  /* make sure we haven't created *too* many windows */
  if (resources.maxWindows && (windowcount >= resources.maxWindows)) {
    static Widget tooManyMsg = 0;
    char message[64];

    if (!tooManyFlag) {
      tooManyFlag = 1;
      sprintf(message, "Can't have more than %d windows!",
	      resources.maxWindows);
      if (!tooManyMsg)
	tooManyMsg = MessageBox(top, message, "OK", popdownTooMany, 0);
      if (tooManyMsg)
	SetPopup(top, tooManyMsg);
    }
    return 1;
  }

  /* get a filehandle (or exit) */
  if (filename) {

#ifdef TILDE_EXPANSION
    /* see if we need to do tilde expansion */
    if (filename && *filename == '~') {
      filename = TildeExpand(filename);
      if (*filename != '~')
	flag |= XLessFreeFilename;
    }
#endif /* TILDE_EXPANSION */

    /* try to open the file */
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
      CouldntOpen(top, filename);
      return 2;
    }

    fixedName = filename;

  } else if (windowcount == 0) {

    /* first file can come from stdin */
    fixedName = "stdin";
    fd = 0;

  } else {

    /* don't let 'em get away with this!!! */
    if (!badFileMsg)
      badFileMsg = MessageBox(top, "Please specify a file name!",
			      "OK", 0, 0);
    if (badFileMsg)
      SetPopup(top, badFileMsg);
    return 3;

  }

  /* keep track of the new window */
  wi = createWindowInfo();
  wi->file = fixedName;
  wi->flag = flag;

  /* read the file into memory */
  InitData(fd, wi);

  /* figure out what to call the shell */
  if (resources.name != NULL)
    shellName = resources.name;
  else
    shellName = progname;

  /* create a new application shell */
  geom = GetGeometryPosition();

  base = XtVaAppCreateShell(shellName, className, applicationShellWidgetClass,
			    disp,
			    XtNallowShellResize, TRUE,
			    XtNgeometry, geom,
			    XtNiconPixmap,
			       XCreateBitmapFromData(disp, XRootWindow(disp,0),
						     XLessWin_bits,
						     XLessWin_width,
						     XLessWin_height),
			    NULL);

  /* set icon & title name for new window */
  SetXNames(base, fixedName);

  /* create the container for the subwindows */
#ifdef FRAME_IS_FORM
  mainFrame = XtVaCreateManagedWidget("frame", formWidgetClass, base, NULL);
#else
  mainFrame = XtVaCreateManagedWidget("frame", panedWidgetClass, base,
				      XtNorientation, XtorientHorizontal,
				      NULL);
#endif

  /* build widgets for new window */
  wi->base = base;
  wi->text = MakeText(mainFrame, wi, wi->memory, 0);
  wi->toolbox = MakeToolbox(mainFrame, wi, filename);

  /* make sure text window gets all keystrokes */
  XtSetKeyboardFocus(mainFrame, wi->text);

  /* no dialog boxes yet */
  wi->searchPopup = 0;
  wi->newWindowPopup = 0;
  wi->changeFilePopup = 0;

  /* display the window */
  XtPopup(base, XtGrabNone);

  /* add this window to the list */
  wi->next = windowlist;
  windowlist = wi;
  windowcount++;

  SetWMHints(wi);

  return 0;
}

void
DestroyWindowInfo(wi)
WindowInfo *wi;
{
  WindowInfo *prev;

  /* free all window-related widgets */
  XtDestroyWidget(wi->base);

  /* free all existing dialog boxes */
  if (wi->searchPopup)
    XtDestroyWidget(wi->searchPopup);
  if (wi->newWindowPopup)
    XtDestroyWidget(wi->newWindowPopup);
  if (wi->changeFilePopup)
    XtDestroyWidget(wi->changeFilePopup);

  /* free all existing dialog box buffers */
  if (wi->searchBuf)
    XtFree(wi->searchBuf);
  if (wi->newWindowBuf)
    XtFree(wi->newWindowBuf);
  if (wi->changeFileBuf)
    XtFree(wi->changeFileBuf);

  /* free text memory */
  XtFree((char *)wi->memory);

  /* free filename string if it was malloc'd */
  if (wi->flag & XLessFreeFilename)
    XtFree((char *)wi->file);

  /* free any input info */
  if (wi->inputInfo)
    XtFree((char *)wi->inputInfo);

  /* remove from windowlist chain */
  if (windowlist == wi)
    windowlist = wi->next;
  else {
    prev = windowlist;
    while (prev && (prev->next != wi))
      prev = prev->next;
    if (prev)
      prev->next = wi->next;
  }

  /* one less window to worry about... */
  XtFree((char *)wi);
  windowcount--;

  /* nothing else to do if we've closed all the windows */
  if (windowcount == 0) {
    exit(0);
  }
}

void
DestroyAllWindows()
{
  while (windowlist)
    DestroyWindowInfo(windowlist);
}
