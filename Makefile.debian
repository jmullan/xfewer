DESTDIR = 
CC = gcc
DEFINES = -DDEBIAN
INCLUDES = -I/usr/X11R6/include
CFLAGS = -O2 -Wall -fno-strength-reduce $(INCLUDES) $(DEFINES)

OBJS = help.o init.o popup.o util.o callbacks.o window.o main.o actions.o \
	functions.o

LIBS = -lX11 -lXext -lXaw -lICE -lXmu

%.o: %.c
	$(CC) $(CFLAGS) -c $<
	
xfewer: $(OBJS)
	$(CC) $(CFLAGS) -o xfewer $(OBJS) $(LIBS)

install: xfewer
	mkdir -p $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/etc/X11/app-defaults
	install -m 0755 xfewer $(DESTDIR)/usr/bin
	install -c -m 0644 XFewer.ad $(DESTDIR)/etc/X11/app-defaults/XFewer
	install -c -m 0644 XFewer-co.ad $(DESTDIR)/etc/X11/app-defaults/XFewer-color

clean:
	rm -f *.o xfewer core
