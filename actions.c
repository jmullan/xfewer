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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xless-1.5/RCS/actions.c,v 1.6 1994/06/02 20:45:12 dglo Exp $
 */

#include <stdio.h>

#include <X11/X.h>

#include <X11/Intrinsic.h>

#include "xless.h"

static void QuitAction __P((Widget, XButtonEvent *, String *, Cardinal *));
static void HelpAction __P((Widget, XButtonEvent *, String *, Cardinal *));

XtActionsRec actions[] = {
  { "quit",		(XtActionProc )QuitAction },
/* { "search",		(XtActionProc )SearchAction }, */
/* { "search_next",		(XtActionProc )SearchNextAction }, */
/* { "editor",		(XtActionProc )EditorAction }, */
/* { "reload",		(XtActionProc )ReloadAction }, */
/* { "change_file",		(XtActionProc )ChangeFileAction }, */
/* { "new_window",		(XtActionProc )NewWindowAction }, */
  { "help",		(XtActionProc )HelpAction },
};
Cardinal numactions = XtNumber(actions);

static void
QuitAction(w, e, p, n)
Widget w;
XButtonEvent *e;
String *p;
Cardinal *n;
{
  QuitFunction();
}

static void
HelpAction(w, e, p, n)
Widget w;
XButtonEvent *e;
String *p;
Cardinal *n;
{
  WindowInfo *wi;

  wi = findWindowInfo(w);
  PopupHelp(w, (XtPointer )wi, NULL);
}
