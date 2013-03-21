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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xfewer-1.7/RCS/util.c,v 1.22 1994/06/16 22:33:46 dglo Exp $
 */


#include <stdio.h>

#include <X11/X.h>
#include <X11/Xos.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include <X11/Xaw/Text.h>

#include "xfewer.h"

XFontStruct *buttonFont;
XFontStruct *labelFont;
XFontStruct *textFont;
Cursor stdcur;
Cursor dialogcur;

static void popdownCouldntOpen __P((Widget, XtPointer, XtPointer));

static struct opendata {
  Widget errbox;
  char *errmsg;
} data;

/*
 *	Function name:	SetPopup
 *	Description:	This function pops up the specified dialog box.
 *	Arguments:	top - where to pop up this dialog box
 *	                wdg - the widget to be popped up.
 *	Returns:	None.
 */

void
SetPopup(top, wdg)
Widget top, wdg;
{
  Position x=0, y=0;
  Window rwin;
  Window chwin;
  int rx, ry, wx, wy;
  Dimension wd = 0;
  Dimension he = 0;
  unsigned int mask;

  /* Make the popup shell "wdg" come up at the current pointer position */
  XQueryPointer(XtDisplay(top), XtWindow(top), &rwin, &chwin, &rx, &ry,
		&wx, &wy, &mask);

  XtVaGetValues(wdg,
		XtNheight, &he,
		XtNwidth, &wd,
		NULL);

  if (wd == 0) wd = XFEWER_DIALOG_WIDTH;
  if (he == 0) he = XFEWER_DIALOG_HEIGHT;

  x = rx - wd/2;
  y = ry - he/2;

  XtVaSetValues(wdg,
		XtNx, x,
		XtNy, y,
		NULL);

  /* Popup the widget */
  XtPopup(wdg, XtGrabExclusive);
}

/*
 *	Function name:	CheckFonts
 *	Description:	This function checks the resource DB for the
 *			user specified fonts.
 *	Arguments:	None.
 *	Returns:	None.
 */

void
CheckFonts()
{
  if (!(buttonFont = resources.fonts.button))
    buttonFont = resources.fonts.standard;
  if (!(labelFont = resources.fonts.label))
    labelFont = resources.fonts.standard;
  if (!(textFont = resources.fonts.text))
    textFont = resources.fonts.standard;

  dialogcur = resources.cursors.dialog;
  stdcur = resources.cursors.dialog;

  if (!buttonFont || !labelFont || !textFont) {
    fprintf(stderr, "%s: unable to open any of the specified fonts\n",
	    progname);
    exit(1);
  }
}

static void
popdownCouldntOpen(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown(data.errbox);
  XtDestroyWidget(data.errbox);
  XtFree(data.errmsg);
}

/*
 *	Function name:	CouldntOpen
 *	Description:	This function pops up the "Couldn't open"  dialog box.
 *	Arguments:	wdg - the widget to be popped up on.
 *	Returns:	None.
 */

void
CouldntOpen(top, filename)
Widget top;
const char *filename;
{
  const char *msgpart = "Couldn't open file: ";

  data.errmsg = (char *)XtMalloc((Cardinal )(strlen(msgpart) +
					     strlen(filename) + 1));
  strcpy(data.errmsg, msgpart);
  strcat(data.errmsg, filename);

  data.errbox = MessageBox(top, data.errmsg, "OK", popdownCouldntOpen, 0);
  if (data.errbox)
    SetPopup(top, data.errbox);
}

#ifdef TILDE_EXPANSION

#include <pwd.h>

#define USERNAMELEN	9

const char *
TildeExpand(filename)
const char *filename;
{
  struct passwd *pw;
  char username[USERNAMELEN], *bptr = username;
  const char *end;
  int len;

  /* find end of tilde'd name */
  end = strchr(filename, '/');
  if (!end)
    end = filename + strlen(filename);

  /* if it's just '~/...' or '~' */
  if (end == filename + 1) {

    /* look up this UID in passwd file */
    pw = getpwuid(getuid());

  } else {

    /* allocate a buffer if static one is too small */
    len = end - filename;
    if (len > USERNAMELEN)
      bptr = (char *)XtMalloc((Cardinal )len);

    /* copy name into buffer */
    len--;
    strncpy(bptr, filename + 1, (size_t )len);
    bptr[len] = 0;

    /* look up this user in passwd file */
    pw = getpwnam(bptr);

    /* free allocated memory */
    if (bptr != username)
      XtFree(bptr);
  }

  /* if we found a passwd entry... */
  if (pw) {

    /* get enough memory for expanded string */
    bptr = (char *)XtMalloc((Cardinal )(strlen(pw->pw_dir) + strlen(end) + 1));

    /* create new string */
    if (bptr) {
      strcpy(bptr, pw->pw_dir);
      strcat(bptr, end);
      filename = bptr;
    }
  }

  /* return final string */
  return(filename);
}
#endif /* TILDE_EXPANSION */
