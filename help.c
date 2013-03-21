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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xless-1.7/RCS/help.c,v 1.23 1994/07/29 02:27:40 dglo Exp $
 */

#include <fcntl.h>

#include <X11/X.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Paned.h>

#include "xless.h"
#include "XLessHlp.icn"

static Widget createHelp __P((Widget));
static void PopdownHelp __P((Widget, XtPointer, XtPointer));

/*	Function Name: CreateHelp.
 *	Description: This function creates the help widget so that it will be
 *	             ready to be displayed.
 *	Arguments:
 *	Returns: the help widget
 */

static Widget
createHelp(top)
Widget top;
{
  static const char *helpIconName = NULL;
  int fd;
  const char *helpPage;
  Widget base, pane, helpText, button;
  XtCallbackRec callback[2];
  XtAccelerators accel;
  const String quitstr = "#override <Key>Q: set() notify() unset()\n";

  /* try to open the help file */
  fd = open(resources.helpFile, O_RDONLY);
  if (fd == -1) {
    CouldntOpen(top, resources.helpFile);
    return 0;
  }

  helpPage = InitData(fd, NULL);

  /* make sure the window title exists */
  if (!helpIconName) {
    char *hin;

    hin = (char *)XtMalloc((Cardinal )(strlen(progname) + 6));
    strcpy(hin, progname);
    strcat(hin, ":help");
    helpIconName = hin;
  }

  /* create a new application shell */
  base = XtVaAppCreateShell("help", className, applicationShellWidgetClass,
			    disp,
			    XtNallowShellResize, TRUE,
			    XtNiconName, helpIconName,
			    XtNiconPixmap,
			      XCreateBitmapFromData(disp, XRootWindow(disp, 0),
						    XLessHelp_bits,
						    XLessHelp_width,
						    XLessHelp_height),
			    NULL);

  pane = XtVaCreateManagedWidget("helpPane", panedWidgetClass, base,
				 NULL);

  helpText = MakeText(pane, NULL, helpPage, 1);

  /* set up Done button callback array */
  callback[0].callback = PopdownHelp;
  callback[0].closure = (XtPointer)base;
  callback[1].callback = NULL;
  callback[1].closure = (XtPointer)NULL;

  /* set up Done button accelerator */
  accel = XtParseAcceleratorTable(quitstr);

  /* create Done button */
  button = XtVaCreateManagedWidget("helpQuit", commandWidgetClass, pane,
				   XtNaccelerators, accel,
				   XtNcallback, callback,
				   XtNfont, buttonFont,
				   XtNlabel, "Done With Help",
				   NULL);

  XtInstallAccelerators(helpText, button);

  XtRealizeWidget(base);

  return(base);
}

/*	Function Name: PopdownHelp
 *	Description: This function pops down the help widget.
 *	Arguments: w - the widget we are calling back from.
 *	           number - (closure) the number to switch on.
 *	           junk - (call data) not used.
 *	Returns: none.
 */

/* ARGSUSED */

static void
PopdownHelp(w, helpWidget, junk)
Widget w;
XtPointer helpWidget, junk;
{
  XtPopdown((Widget )helpWidget);
}

/*	Function Name: PopupHelp
 *	Description: This function pops up the help widget, unless no
 *	             help could be found.
 *	Arguments: w - the widget we are calling back from.
 *	           number - (closure) the number to switch on.
 *	           junk - (call data) not used.
 *	Returns: none.
 */

/* ARGSUSED */

void
PopupHelp(w, closure, callData)
Widget w;
XtPointer closure, callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  static Widget helpWidget = 0;

  if (!helpWidget)
    helpWidget = createHelp(wi ? wi->base : w);
  if (helpWidget)
    XtPopup(helpWidget, XtGrabNone);
}
