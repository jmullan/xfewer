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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xfewer-1.7/RCS/popup.c,v 1.29 1994/06/09 22:59:12 dglo Exp $
 */

#include <stdio.h>

#include <X11/X.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Toggle.h>

#include "xfewer.h"

#define SEARCH_BITS	((unsigned )(XFewerSearchInsensitive|XFewerSearchRegExpr))

/* stole this chunk from xdm/dm.c */
#if defined(USE_PROTOTYPES) || defined(__STDC__)
#include <stdarg.h>
#define Va_start(a,b)	va_start(a,b)
#else
#include <varargs.h>
#define Va_start(a,b)	va_start(a)
#endif

static void toggleSearchType __P((Widget, XtPointer, XtPointer));
static void popdownBox __P((Widget, XtPointer, XtPointer));

Widget
DialogBox(top, confFunc, confData, label, btnLabel, selection)
Widget top;
XtCallbackProc confFunc;
XtPointer confData;
const char *label;
const char *btnLabel;
char *selection;
{
  static XtActionsRec dbactions[2];
  static String myreturn = "#override <Key>Return:   set() notify() unset()\n";
  XtAccelerators accel;
  static String mytranslations = "#override\n\
	Ctrl<Key>S:  no-op(RingBell)\n\
	Ctrl<Key>R:  no-op(RingBell)\n";
  XtTranslations xlate;
  static int init = 0;
  Widget popup, form, lbl, input, confirm, cancel;
  XtCallbackRec callback[2];

  dbactions[0].string = "Nothing";
  dbactions[0].proc = (XtActionProc)NULL;

  if (!init) {
    XtAddActions(dbactions, XtNumber(dbactions));
    init = 1;
  }

  callback[1].callback = NULL;
  callback[1].closure = (XtPointer)NULL;

  popup = XtVaCreatePopupShell("popup", transientShellWidgetClass, top,
			       XtNallowShellResize, True,
			       NULL);

  form = XtVaCreateManagedWidget("form", formWidgetClass, popup,
				 NULL);

  lbl = XtVaCreateManagedWidget(label, labelWidgetClass, form,
				XtNborderWidth, 0,
				XtNfont, labelFont,
				XtNjustify, XtJustifyLeft,
				XtNlabel, label,
				XtNleft, XtChainLeft,
				XtNright, XtChainLeft,
				NULL);

  xlate = XtParseTranslationTable(mytranslations);
  input = XtVaCreateManagedWidget("input", asciiTextWidgetClass, form,
				  XtNeditType, XawtextEdit,
				  XtNfont, textFont,
				  XtNfromHoriz, NULL,
				  XtNfromVert, lbl,
				  XtNleft, XtChainLeft,
				  XtNlength, XFEWER_MAX_INPUT,
				  XtNresizable, True,
				  XtNright, XtChainRight,
				  XtNstring, selection,
#ifdef X11R4
				  XtNtextOptions, resizeWidth,
#endif
				  XtNtranslations, xlate,
				  XtNuseStringInPlace, True,
				  XtNvertDistance, 0,
				  XtNwidth, XFEWER_INPUT_WIDTH,
				  NULL);

  XtSetKeyboardFocus(form, input);

  accel = XtParseAcceleratorTable(myreturn);
  callback[0].callback = confFunc;
  callback[0].closure = confData;
  confirm = XtVaCreateManagedWidget(btnLabel, commandWidgetClass, form,
				    XtNaccelerators, accel,
				    XtNcallback, callback,
				    XtNcursor, dialogcur,
				    XtNfont, buttonFont,
				    XtNfromVert, input,
				    XtNfromHoriz, NULL,
				    XtNleft, XtChainLeft,
				    XtNright, XtChainLeft,
				    XtNvertDistance, 5,
				    XtNwidth, XFEWER_BUTTON_WIDTH,
				    NULL);

  callback[0].callback = Cancel;
  callback[0].closure = (XtPointer)popup;
  cancel = XtVaCreateManagedWidget("Cancel", commandWidgetClass, form,
				   XtNcallback, callback,
				   XtNcursor, dialogcur,
				   XtNfont, buttonFont,
				   XtNfromVert, input,
				   XtNfromHoriz, confirm,
				   XtNhorizDistance, 2,
				   XtNleft, XtChainRight,
				   XtNright, XtChainRight,
				   XtNvertDistance, 5,
				   XtNwidth, XFEWER_BUTTON_WIDTH,
				   NULL);

  XtInstallAccelerators(input, confirm);

  return(popup);
}

static void
toggleSearchType(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  int searchType = wi->flag & SEARCH_BITS;

  if (!searchType)
    wi->flag |= XFewerSearchInsensitive;
  else if (searchType && ((searchType & XFewerSearchInsensitive) == searchType))
    wi->flag = (wi->flag & ~SEARCH_BITS) | XFewerSearchRegExpr;
  else if (searchType && ((searchType & XFewerSearchRegExpr) == searchType))
    wi->flag &= ~SEARCH_BITS;
  else
    wi->flag = (wi->flag & ~SEARCH_BITS) | XFewerSearchInsensitive;

  XtVaSetValues(widget,
		  XtNlabel, (wi->flag & XFewerSearchInsensitive ?
			     " Case Insensitive " :
			     (wi->flag & XFewerSearchRegExpr ?
			     "Regular Expression" :
			     "   Exact Match    ")),
		  NULL);
}

Widget
SearchBox(top, srchFunc, wi, label, btnLabel, selection)
Widget top;
XtCallbackProc srchFunc;
WindowInfo *wi;
const char *label;
const char *btnLabel;
char *selection;
{
  static XtActionsRec dbactions[2];
  static String myreturn = "#override <Key>Return:   set() notify() unset()\n";
  XtAccelerators accel;
  static String mytranslations = "#override\n\
	Ctrl<Key>S:  no-op(RingBell)\n\
	Ctrl<Key>R:  no-op(RingBell)\n";
  XtTranslations xlate;
  static int init = 0;
  Widget popup, form, lbl, input, searchType, search, cancel;
  const char *searchLabel;
  XtCallbackRec callback[2];

  dbactions[0].string = "Nothing";
  dbactions[0].proc = (XtActionProc)NULL;

  if (!init) {
    XtAddActions(dbactions, XtNumber(dbactions));
    init = 1;
  }

  callback[1].callback = NULL;
  callback[1].closure = (XtPointer)NULL;

  popup = XtVaCreatePopupShell("popup", transientShellWidgetClass, top,
			       XtNallowShellResize, True,
			       NULL);

  form = XtVaCreateManagedWidget("form", formWidgetClass, popup,
				 NULL);

  lbl = XtVaCreateManagedWidget(label, labelWidgetClass, form,
				XtNborderWidth, 0,
				XtNfont, labelFont,
				XtNjustify, XtJustifyLeft,
				XtNlabel, label,
				XtNleft, XtChainLeft,
				XtNright, XtChainLeft,
				NULL);

  xlate = XtParseTranslationTable(mytranslations);
  input = XtVaCreateManagedWidget("input", asciiTextWidgetClass, form,
				  XtNeditType, XawtextEdit,
				  XtNfont, textFont,
				  XtNfromHoriz, NULL,
				  XtNfromVert, lbl,
				  XtNleft, XtChainLeft,
				  XtNlength, XFEWER_MAX_INPUT,
				  XtNresizable, True,
				  XtNright, XtChainRight,
				  XtNstring, selection,
#ifdef X11R4
				  XtNtextOptions, resizeWidth,
#endif
				  XtNtranslations, xlate,
				  XtNuseStringInPlace, True,
				  XtNvertDistance, 0,
				  XtNwidth, XFEWER_INPUT_WIDTH,
				  NULL);

  XtSetKeyboardFocus(form, input);

  wi->flag &= ~SEARCH_BITS;
  if (resources.defaultSearchType == XFewerSearchInsensitive) {
    searchLabel = "      Case Insensitive     ";
    wi->flag |= XFewerSearchInsensitive;
  } else if (resources.defaultSearchType == XFewerSearchRegExpr) {
    searchLabel = "     Regular Expression    ";
    wi->flag |= XFewerSearchRegExpr;
  } else {
    searchLabel = "        Exact Match        ";
  }

  callback[0].callback = toggleSearchType;
  callback[0].closure = (XtPointer )wi;
  searchType = XtVaCreateManagedWidget("searchType", commandWidgetClass, form,
				       XtNcallback, callback,
				       XtNfont, buttonFont,
				       XtNfromVert, input,
				       XtNfromHoriz, NULL,
				       XtNlabel, searchLabel,
				       XtNleft, XtChainLeft,
				       XtNright, XtChainRight,
				       NULL);

  accel = XtParseAcceleratorTable(myreturn);

  callback[0].callback = srchFunc;
  callback[0].closure = (XtPointer )wi;
  search = XtVaCreateManagedWidget(btnLabel, commandWidgetClass, form,
				   XtNaccelerators, accel,
				   XtNcallback, callback,
				   XtNcursor, dialogcur,
				   XtNfont, buttonFont,
				   XtNfromVert, searchType,
				   XtNfromHoriz, NULL,
				   XtNleft, XtChainLeft,
				   XtNright, XtChainLeft,
				   XtNvertDistance, 5,
				   XtNwidth, XFEWER_BUTTON_WIDTH,
				   NULL);

  callback[0].callback = Cancel;
  callback[0].closure = (XtPointer)popup;
  cancel = XtVaCreateManagedWidget("Cancel", commandWidgetClass, form,
				   XtNcallback, callback,
				   XtNcursor, dialogcur,
				   XtNfont, buttonFont,
				   XtNfromVert, searchType,
				   XtNfromHoriz, search,
				   XtNhorizDistance, 2,
				   XtNleft, XtChainRight,
				   XtNright, XtChainRight,
				   XtNvertDistance, 5,
				   XtNwidth, XFEWER_BUTTON_WIDTH,
				   NULL);

  XtInstallAccelerators(input, search);

  return(popup);
}

static void
popdownBox(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown((Widget)closure);
}

/*
 *	Function Name:	MessageBox
 *	Description:	This function creates the message dialog box.
 *	Arguments:  top		widget to use as message box parent
 *		    msg		message to display
 *		    btnlabel	label on next button
 *		    btnproc	callback executed when next button is pressed
 *	Returns:    widget	id of the message box.
 */

Widget
#if defined(USE_PROTOTYPES) || defined(__STDC__)
MessageBox(Widget top, const char *msg, ...)
#else
MessageBox(top, msg, va_alist)
Widget top;
const char *msg;
va_dcl
#endif
{
  Widget msgPop, form, msgLabel, button;
  XtCallbackRec callback[2];
  static String myreturn = "#override <Key>Return: set() notify() unset()\n";
  XtAccelerators accel;
  const char *btntext;
  XtCallbackProc btnproc;
  va_list vap;

  /* can't popup a messagebox if widgets haven't been realized yet... */
  if (!XtIsRealized(top)) {
    XtAppWarning(context, msg);
    return 0;
  }

  callback[1].callback = (XtCallbackProc) NULL;
  callback[1].closure = (XtPointer) NULL;

  msgPop = XtVaCreatePopupShell("msgPop", transientShellWidgetClass, top,
				XtNallowShellResize, True,
				NULL);

  form = XtVaCreateManagedWidget("form", formWidgetClass, msgPop,
				 XtNallowShellResize, True,
				 NULL);

  msgLabel = XtVaCreateManagedWidget("message", labelWidgetClass, form,
				     XtNallowShellResize, True,
				     XtNborderWidth, 0,
				     XtNfont, labelFont,
				     XtNjustify, XtJustifyLeft,
				     XtNlabel, msg,
				     NULL);

  Va_start(vap, msg);
  btntext = (const char *)va_arg(vap, const char *);

  accel = XtParseAcceleratorTable(myreturn);

  button = 0;
  while (btntext) {
    btnproc = (XtCallbackProc )va_arg(vap, XtCallbackProc);

    if (btnproc)
      callback[0].callback = btnproc;
    else
      callback[0].callback = popdownBox;
    callback[0].closure = (XtPointer)msgPop;
    button = XtVaCreateManagedWidget(btntext, commandWidgetClass, form,
				     XtNaccelerators, accel,
				     XtNcallback, callback,
				     XtNcursor, dialogcur,
				     XtNfont, buttonFont,
				     XtNfromVert, msgLabel,
				     XtNfromHoriz, button,
				     XtNleft, XtChainRight,
				     XtNright, XtChainRight,
				     XtNvertDistance, 1,
				     XtNwidth, XFEWER_BUTTON_WIDTH,
				     NULL);

    XtInstallAccelerators(form, button);

    btntext = (const char *)va_arg(vap, const char *);
  }

  return(msgPop);
}
