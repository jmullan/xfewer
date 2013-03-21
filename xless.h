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
 *	$Header: /usr/sww/share/src/X11R6/local/applications/xless-1.7/RCS/xless.h,v 1.53 1994/07/29 02:55:34 dglo Exp $
 *
 */

#ifndef XLESS_H
#define XLESS_H

/* Useful defines */

#define XLESS_MEMORY_CHUNK	512000

#define XLESS_BUTTON_WIDTH	100

#define XLESS_DIALOG_WIDTH	350
#define XLESS_DIALOG_HEIGHT	150

#define XLESS_INPUT_WIDTH	200

#define XLESS_MAX_INPUT		256

/* handle differences between K&R and ANSI C */
#ifndef __P
#if __STDC__
#define __P(a)	a
#else
#define __P(a)	()
#define const
#endif
#endif

/*
 * The default editor; make sure the file exists.
 * (Used if the environment variable EDITOR is not set)
 */

#ifndef DEFEDITOR
#define DEFEDITOR "/etc/alternatives/editor"
#endif

/*
 * This is the default helpfile and is site specific.
 * If you're not using 'imake', make sure you change this.
 */

#ifndef HELPFILE
#define HELPFILE "/usr/share/doc/xless/xless.help"
#endif

/*
 * This is the default print command and is OS specific.
 * It should probably be "lpr" for BSD, "lp" for SysV
 */

#ifndef PRINTCMD
#define PRINTCMD "/usr/bin/lpr"
#endif

/*
 * Application class.  You shouldn't change this.
 */
#ifndef XLESS_CLASS
#define XLESS_CLASS "XLess"
#endif /* XLESS_CLASS */

/* Default fonts. */
#define STANDARDCUR "left_ptr"

#define STANDARDFONT "fixed"
#define TEXTFONT "-adobe-courier-medium-r-normal--12-120-75-75-m-*-iso8859-1"
#define LABELFONT "-adobe-times-medium-r-normal--12-120-75-75-p-*-iso8859-1"
#define BUTTONFONT "-adobe-new century schoolbook-medium-r-normal--12-120-75-75-p-*-iso8859-1"

/* Typedefs */

/*
 * Values for WindowInfo flag
 */
typedef enum _XLessFlagBits {
  XLessClearFlag = 0x0000,		/* turn off all bits */
  XLessFreeFilename = 0x0001,		/* filename uses malloc'd memory */
  XLessAddedNewline = 0x0002,		/* added newline to end of text */
  XLessSearchInsensitive = 0x0100,	/* case-insensitive search */
  XLessSearchRegExpr = 0x0200		/* regular-expression search */
} XLessFlag;

/*
 * This structure holds info needed to stop and start input
 */
typedef struct _InputInfo {
  int fd;
  XtInputId id;
  int falseAlarm;
  unsigned long interval;
} InputInfo;

/*
 * This structure holds everything xless needs to know about
 * each of its windows.
 */
typedef struct _WindowInfo {

  /* main window widgets */
  Widget base;
  Widget text;
  Widget toolbox;
  Widget editorButton;
  Widget reloadButton;

  /* popup window widgets and buffers */
  Widget searchPopup, newWindowPopup, changeFilePopup;
  char *searchBuf, *newWindowBuf, *changeFileBuf;

  /* actual text and associated statistics */
  const char *memory;
  Cardinal allocated, used;

  /* file name (if any) */
  const char *file;

  /* miscellaneous flag values */
  XLessFlag flag;

  /* height (in lines), width (in chars) */
  unsigned dataHeight, dataWidth;

  /* info needed to manage dynamic files */
  InputInfo *inputInfo;

  /* next in the series (collect 'em all!) */
  struct _WindowInfo *next;

} WindowInfo;

typedef struct _XLessFonts {
  XFontStruct
    *standard,	/* The font used if no fonts specied */
    *text,			/* Font used for XLess text */
    *label,			/* Font used for labels in dialog boxes */
    *button;		/* Font used for commandbuttons */
} XLessFonts;

typedef struct _XLessCursors {
  Cursor
    top,			/* The top cursor, default for XLess */
    dialog;			/* Cursor for dialog boxes */
} XLessCursors;

/* Resource manager sets these */

typedef struct _XLessResources {
  String geometry;		/* width/height of text window */
  String name;			/* instance name */
  String title;			/* title name */
  XLessFonts fonts;		/* The fonts used for XLess */
  XLessCursors cursors;		/* The cursors used for XLess */
  String helpFile;		/* The help file name */
  Boolean editorDoesWindows;	/* does editor come up in a window? */
  String editor;		/* default editor */
  String printCmd;		/* command used to print file */
  int maxWindows;		/* maximum number of open windows */
  Boolean quitButton;		/* does user want a QUIT button? */
  Boolean autoName;		/* should we automatically name the window? */
  Boolean removePath;		/* does user want path junk removed? */
  Boolean sizeToFit;		/* should window be as small as possible? */
  Boolean helpMessage;		/* should we just print a help msg and exit? */
  unsigned defaultSearchType;	/* flag used to set default search type */
  Boolean monitorFile;		/* should we keep checking for input? */
  Boolean printVersion;		/* should we keep print our version number? */
  String oldPrintCmd;		/* catch old resource name usage */
} XLessResources;

/* prototypes from help.c */
void PopupHelp __P((Widget, XtPointer, XtPointer));

/* prototypes from init.c */
const char *InitData __P((int, WindowInfo *));
Widget MakeToolbox __P((Widget, WindowInfo *, const char *));
const char *GetGeometryPosition __P((void));
Widget MakeText __P((Widget, WindowInfo *, const char *, int));
void SetXNames __P((Widget, const char *));
void SetWMHints __P((WindowInfo *));

/* prototypes from popup.c */
Widget DialogBox __P((Widget, XtCallbackProc, XtPointer, const char *,
		       const char *, char *));
Widget SearchBox __P((Widget, XtCallbackProc, WindowInfo *, const char *,
		       const char *, char *));
Widget MessageBox __P((Widget, const char *, ...));
WindowInfo *MakeDialog __P((Widget));

/* prototypes from util.c */
void SetPopup __P((Widget, Widget));
void CheckFonts __P((void));
void CouldntOpen __P((Widget, const char *));
const char *TildeExpand __P((const char *));
extern XFontStruct *buttonFont;
extern XFontStruct *labelFont;
extern XFontStruct *textFont;
extern Cursor stdcur;
extern Cursor dialogcur;

/* prototypes from callbacks.c */
void Quit __P((Widget, XtPointer, XtPointer));
void SearchNext __P((Widget, XtPointer, XtPointer));
void Cancel __P((Widget, XtPointer, XtPointer));
void CallEditor __P((Widget, XtPointer, XtPointer));
void Reload __P((Widget, XtPointer, XtPointer));
void Search __P((Widget, XtPointer, XtPointer));
void ChangeFile __P((Widget, XtPointer, XtPointer));
void NewWindow __P((Widget, XtPointer, XtPointer));
void CloseWindow __P((Widget, XtPointer, XtPointer));
void Print __P((Widget, XtPointer, XtPointer));
void Background __P((Widget, XtPointer, XtPointer));

/* prototypes from functions.c */
void QuitFunction __P((void));

/* prototypes from main.c */
int main __P((int, char *[]));
extern XtAppContext context;
extern Widget toplevel;
extern Display *disp;
extern XLessResources resources;
extern const char *progname;
extern const char *className;

/* prototypes from window.c */
WindowInfo *createWindowInfo __P((void));
WindowInfo *findWindowInfo __P((Widget));
int CreateWindow __P((Widget, const char *));
void DestroyWindowInfo __P((WindowInfo *));
void DestroyAllWindows __P((void));
extern WindowInfo *windowlist;
extern int windowcount;

/* prototypes from actions.c */
extern XtActionsRec actions[];
extern Cardinal numactions;

#endif /* XLESS_H */
