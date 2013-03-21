#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Core.h>
#include <X11/Shell.h>

extern Widget toplevel;

void
widgdump(w)
Widget w;
{
  WidgetClass wclass;
  char *name;

  while (w) {
    wclass = XtClass(w);
    printf("Widget \"%s\" is class \"%s\"\n",
	   XtName(w), wclass->core_class.class_name);
    if (wclass == applicationShellWidgetClass) {
      if (w == toplevel)
	printf("^^^^TOP^^^^\n");
      else
	printf("^^WINDOW^^^\n");
    }
    w = XtParent(w);
  }
}
