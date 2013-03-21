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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xless-1.5/RCS/functions.c,v 1.11 1994/06/02 20:45:12 dglo Exp $
 */

#include <stdio.h>

#include <X11/X.h>

#include <X11/Intrinsic.h>

#include "xless.h"

static void okQuitAll __P((Widget, XtPointer, XtPointer));
static void cancelQuitAll __P((Widget, XtPointer, XtPointer));
static int QuitAllPrompt __P((void));

static void
okQuitAll(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown((Widget)closure);
  XtDestroyWidget((Widget)closure);
  DestroyAllWindows();
  exit(0);
}

static void
cancelQuitAll(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown((Widget)closure);
}

static int
QuitAllPrompt()
{
  Widget base;
  static Widget quitall = 0;

  /* make sure there's at least one window */
  if (!windowlist)
    return(1);

  /* grab first base widget */
  base = windowlist->base;

  /* popup warning box */
  if (!quitall)
    quitall = MessageBox(base, "Quit ALL windows?",
		       "Cancel", cancelQuitAll, "OK", okQuitAll, 0);
  SetPopup(base, quitall);
  return(0);
}

/*
 * clean up, exit application
 */
void
QuitFunction()
{
  if ((windowcount == 1) || QuitAllPrompt()) {
    DestroyAllWindows();
    exit(0);
  }
}
