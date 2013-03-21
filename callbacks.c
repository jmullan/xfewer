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
 * $Header: /usr/sww/share/src/X11R6/local/applications/xless-1.7/RCS/callbacks.c,v 1.37 1994/07/29 02:28:14 dglo Exp $
 */

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>

#include <X11/X.h>
#include <X11/Xos.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include <X11/Xaw/AsciiText.h>

#ifdef DEBIAN
#define __DO_NOT_DEFINE_COMPILE
#include <regexp.h>
#else
#include "regexp/regexp.h"
#endif
#include "xless.h"

extern char *getenv __P((const char *));
extern int system __P((const char *));

static void getReadOnlySource __P((Widget, const char *));
static int find __P((const char *, WindowInfo *, XawTextPosition));
static void doSearch __P((WindowInfo *));
static void popdownAndSearch __P((Widget, XtPointer, XtPointer));
static void popdownAndChange __P((Widget, XtPointer, XtPointer));
static void popdownAndCreate __P((Widget, XtPointer, XtPointer));

static void
getReadOnlySource(w, string)
Widget w;
const char *string;
{
  Widget source;

  /* free old source */
  source = XawTextGetSource(w);
  XtDestroyWidget(source);

  /* Create a new source for the text widget, and put in the new string */
  source = XtVaCreateWidget("readText", asciiSrcObjectClass, w,
			    XtNeditType, XawtextRead,
			    XtNstring, string,
			    XtNuseStringInPlace, True,
			    NULL);

  XawTextSetSource(w, source, (XawTextPosition) 0);
}

/*
 * Button callback functions.
 */

void
Quit(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  QuitFunction();
}

void
Cancel(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  XtPopdown((Widget)closure);
}

void
CallEditor(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  const char *editor;
  int cmdlen = 0;
  char *cmd;

  /* figure out which editor they want */
  if ((editor = getenv("VISUAL")) == NULL &&
      (editor = getenv("EDITOR")) == NULL)
    editor = (resources.editor ? resources.editor : DEFEDITOR);

  /* get a buffer long enough for the entire command */
  if (!resources.editorDoesWindows)
    cmdlen = 9;			/* "xterm -e " */
  cmdlen += strlen(editor) + 1 + strlen(wi->file) + 2;
  cmd = (char *)XtMalloc((Cardinal )cmdlen);

  /* don't start from an xterm if the editor has its own window */
  if (resources.editorDoesWindows)
    strcpy(cmd, editor);
  else {
    strcpy(cmd, "xterm -e ");
    strcat(cmd, editor);
  }
  strcat(cmd, " ");
  strcat(cmd, wi->file);
  strcat(cmd, "&");
  system(cmd);
  XtFree(cmd);
}

void
Reload(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  int fd;

  fd = open(wi->file, O_RDONLY);
  if (fd == -1)
    CouldntOpen(wi->base, wi->file);
  else {
    XtFree((char *)wi->memory);
    InitData(fd, wi);
    getReadOnlySource(wi->text, wi->memory);
  }
}

#ifdef DEBIAN
static int
find(text, wi, offset)
const char *text;
WindowInfo *wi;
XawTextPosition offset;
{
  static Widget badPattern = NULL;
  char *str = wi->searchBuf;
  XawTextPosition beg, end;
  regex_t pat;
  regmatch_t pmatch;
  int rtnval;

  /* try to match the pattern */
  rtnval = 0;
  if (regcomp(&pat, str, REG_EXTENDED) != 0) {
    if (!badPattern)
      badPattern = MessageBox(wi->base, "Bad pattern ...", "OK", 0, 0);
    if (badPattern)
      SetPopup(wi->base, badPattern);
  } else if ((regexec(&pat, text, 1, &pmatch, 0) == 0) && (pmatch.rm_so != -1)) {
    beg = offset + pmatch.rm_so;
    end = offset + pmatch.rm_eo;
    XawTextSetInsertionPoint(wi->text, end);
    XawTextSetSelection(wi->text, beg, end);
  } else
    rtnval = -1;

  return(rtnval);
}
#else
static int
find(text, wi, offset)
const char *text;
WindowInfo *wi;
XawTextPosition offset;
{
  const char *special = "\\|*+?.^$[]()";
  static Widget badPattern = NULL;
  char *str = wi->searchBuf;
  XawTextPosition beg, end;
  regexp *pat;
  char *s;
  Boolean insensitive, allocated;
  int len;
  unsigned slen, nSpecial, nAlpha;
  char *newstr;
  int rtnval;

  /* is this a case-insensitive search? */
  insensitive = (wi->flag & XLessSearchInsensitive) == XLessSearchInsensitive;
  allocated = 0;

  /* if we need to escape special characters... */
  if ((wi->flag & XLessSearchRegExpr) != XLessSearchRegExpr) {

    /* count the special characters */
    nSpecial = nAlpha = 0;
    for (s = str; *s; s++) {
      if (strchr(special, *s) != NULL)
	nSpecial++;
      else if (insensitive && isalpha(*s))
	nAlpha++;
    }

    /* if we need to escape something... */
    if (nSpecial > 0 || (insensitive && nAlpha > 0)) {

      /* figure out how much space we'll need */
      len = strlen(str);
      if (insensitive)
	slen = nAlpha * 4 + nSpecial * 2 + (len - (nSpecial + nAlpha));
      else
	slen = len + nSpecial;

      /* get space for the new string */
      newstr = (char *)XtMalloc(slen + 1);
      if (newstr == NULL) {
	fprintf(stderr, "Out of space in search routine!\n");
	exit(1);
      }

      /* build the special string */
      for (s = newstr; *str; str++) {
	if (strchr(special, *str) != NULL) {
	  *s++ = '\\';
	  *s++ = *str;
	} else if (insensitive && isalpha(*str)) {
	  *s++ = '[';
	  *s++ = *str;
	  if (isupper(*str))
	    *s++ = tolower(*str);
	  else
	    *s++ = toupper(*str);
	  *s++ = ']';
	} else
	  *s++ = *str; 
      }
      *s = 0;

      /* allocated string is the search pattern now */
      allocated = 1; 
      str = newstr;
    }
  }

  /* try to match the pattern */
  rtnval = 0;
  pat = regcomp(str);
  if (pat == NULL) {
    if (!badPattern)
      badPattern = MessageBox(wi->base, "Bad pattern ...", "OK", 0, 0);
    if (badPattern)
      SetPopup(wi->base, badPattern);
  } else if (regexec(pat, text)) {
    beg = offset + (pat->startp[0] - text);
    end = beg + (pat->endp[0] - pat->startp[0]);
    XawTextSetInsertionPoint(wi->text, end);
    XawTextSetSelection(wi->text, beg, end);
  } else
    rtnval = -1;

  /* clean up allocated memory */
  if (allocated)
    XtFree(str);
  if (pat)
    XtFree((char *)pat);

  return(rtnval);
}
#endif

static void
doSearch(wi)
WindowInfo *wi;
{
  XawTextPosition offset;
  int len;
  const char *top;
  static Widget notFound = NULL;

  len = strlen(wi->searchBuf);
  offset = XawTextGetInsertionPoint(wi->text);
  top = wi->memory + offset;
  if (len <= 0 || find(top, wi, offset)) {
    if (!notFound)
      notFound = MessageBox(wi->base, "String not found ...", "OK", 0, 0);
    if (notFound)
      SetPopup(wi->base, notFound);
  }
}

void
SearchNext(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  static Widget noString = NULL;

  if (wi->searchBuf == NULL) {
    if (!noString)
      noString = MessageBox(wi->base, "No search string specified ...",
			    "OK", 0, 0);
    if (noString)
      SetPopup(wi->base, noString);
  } else
    doSearch(wi);
}

static void
popdownAndSearch(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;

  XtPopdown((Widget)wi->searchPopup);
  doSearch(wi);
}

void
Search(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;

  if (!wi->searchBuf) {
    wi->searchBuf = (char *)XtMalloc(XLESS_MAX_INPUT);
    wi->searchBuf[0] = 0;
  }

  if (!wi->searchPopup)
    wi->searchPopup = SearchBox(wi->base, popdownAndSearch, (XtPointer )wi,
				"Search for:", "Search", wi->searchBuf);

  /* popup search dialog box */
  SetPopup(wi->base, wi->searchPopup);
}

static void
popdownAndChange(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  int fd;
  const char *filename;
  XLessFlag flag = XLessClearFlag;

  XtPopdown((Widget)wi->changeFilePopup);

  /* make sure there's a filename there */
  filename = wi->changeFileBuf;
  if (*filename == 0)
    return;

#ifdef TILDE_EXPANSION
    /* see if we need to do tilde expansion */
    if (filename && *filename == '~') {
      filename = TildeExpand(filename);
      if (*filename != '~')
	flag |= XLessFreeFilename;
    }
#endif /* TILDE_EXPANSION */

  fd = open(filename, O_RDONLY);
  if (fd == -1)
    CouldntOpen(wi->base, filename);
  else {

    /* read in new file */
    XtFree((char *)wi->memory);
    InitData(fd, wi);
    if (wi->flag & XLessFreeFilename) {
      XtFree((char *)wi->file);
      wi->flag &= ~XLessFreeFilename;
    }
    getReadOnlySource(wi->text, wi->memory);
    wi->file = filename;
    wi->flag |= flag;

    /* sensitize buttons if previous file was STDIN */
    if (wi->editorButton) {
      XtVaSetValues(wi->editorButton, XtNsensitive, True, NULL);
      wi->editorButton = 0;
    }
    if (wi->reloadButton) {
      XtVaSetValues(wi->reloadButton, XtNsensitive, True, NULL);
      wi->editorButton = 0;
    }

    /* set title & icon name */
    SetXNames(wi->base, filename);
  }
}

void
ChangeFile(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;

  if (!wi->changeFileBuf) {
    wi->changeFileBuf = (char *)XtMalloc(XLESS_MAX_INPUT);
    wi->changeFileBuf[0] = 0;
  }

  if (!wi->changeFilePopup)
    wi->changeFilePopup = DialogBox(wi->base, popdownAndChange, (XtPointer)wi,
				    "Enter filename:", "Ok",
				    wi->changeFileBuf);

  SetPopup(wi->base, wi->changeFilePopup);	/* Change file dialog box */
}

static void
popdownAndCreate(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;

  XtPopdown((Widget)wi->newWindowPopup);
  CreateWindow(wi->base, wi->newWindowBuf);
}

void
NewWindow(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;

  if (!wi->newWindowBuf) {
    wi->newWindowBuf = (char *)XtMalloc(XLESS_MAX_INPUT);
    wi->newWindowBuf[0] = 0;
  }

  if (!wi->newWindowPopup)
    wi->newWindowPopup = DialogBox(wi->base, popdownAndCreate, (XtPointer)wi,
			"Enter filename:", "Ok", wi->newWindowBuf);
  SetPopup(wi->base, wi->newWindowPopup);	/* New window dialog box */
}

void
CloseWindow(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *sd = (WindowInfo *)closure;

  DestroyWindowInfo(sd);
}

void
Print(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  WindowInfo *wi = (WindowInfo *)closure;
  FILE *file;
  const char *printfile;
  char *cmd;
#ifdef WIMPY_STDIO
  char *line, *l;
#endif

  if (wi->file == NULL) {

    /* create a temporary file */
    printfile = tmpnam(0);
    file = fopen(printfile, "w");

#ifdef WIMPY_STDIO
    /* write stdin (line by line) to temporary file */
    line = l = wi->memory;
    while (line && *line) {
      if (l = strchr(line, '\n'))
	*l = 0;
      fprintf(file, "%s\n", line);
      if (l)
	*l = '\n';
      line = l+1;
    }
#else /* !WIMPY_STDIO */
    /* write stdin (in one manly hunk) to temp file */
    fputs(wi->memory, file);
#endif /* WIMPY_STDIO */
    fclose(file);
  } else
    printfile = wi->file;

  /* create the print command string */
  cmd = (char *)XtMalloc((Cardinal )(strlen(resources.printCmd) + 1 +
				     strlen(printfile) + 1));
  strcpy(cmd, resources.printCmd);
  strcat(cmd, " ");
  strcat(cmd, printfile);
  system(cmd);
  XtFree(cmd);

  /* unlink the file if we created it */
  if (wi->file == NULL)
    unlink(printfile);
}

void
Background(widget, closure, callData)
Widget widget;
XtPointer closure;
XtPointer callData;
{
  static Widget forkError = NULL;
  WindowInfo *wi = (WindowInfo *)closure;

  switch (fork()) {
  case 0:
    break;
  case -1:
    if (!forkError)
      forkError = MessageBox(wi->base, "Error while backgrounding ...",
			    "OK", 0, 0);
    if (forkError)
      SetPopup(wi->base, forkError);
    break;
  default:
    exit(0);
  }
}
