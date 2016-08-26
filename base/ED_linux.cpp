#include "ED_base.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>


global bool gRunning;
global void *gLinuxBitmapMemory;

global const int kWindowWidth = 1024;
global const int kWindowHeight = 768;

global XImage *gXImage;


int main(int argc, char const *argv[]) {
  Display *display;
  Window window;
  int screen;

  display = XOpenDisplay(0);
  if (display == 0) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  screen = DefaultScreen(display);

  u32 border_color = WhitePixel(display, screen);
  u32 bg_color = BlackPixel(display, screen);

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               kWindowWidth, kWindowHeight, 0, border_color,
                               bg_color);

  XSetStandardProperties(display, window, "Editor", "Hi!", None, NULL, 0, NULL);

  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | StructureNotifyMask);
  XMapRaised(display, window);

  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }

    gXImage = XGetImage(display, window, 0, 0, kWindowWidth, kWindowHeight,
                        AllPlanes, ZPixmap);

    gLinuxBitmapMemory = (void *)gXImage->data;

    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  gRunning = true;

  while (gRunning) {
    // Process events
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      KeySym key;
      char buf[256];
      char symbol = 0;
      b32 pressed = false;
      b32 released = false;
      b32 retriggered = false;

      if (XLookupString(&event.xkey, buf, 255, &key, 0) == 1) {
        symbol = buf[0];
      }

      // Process user input
      if (event.type == KeyPress) {
        printf("Key pressed\n");
        pressed = true;
      }

      if (event.type == KeyRelease) {
        if (XEventsQueued(display, QueuedAfterReading)) {
          XEvent nev;
          XPeekEvent(display, &nev);

          if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
              nev.xkey.keycode == event.xkey.keycode) {
            // Ignore. Key wasn't actually released
            printf("Key release ignored\n");
            XNextEvent(display, &event);
            retriggered = true;
          }
        }

        if (!retriggered) {
          printf("Key released\n");
          released = true;
        }
      }

      if (pressed || released) {
        if (key == XK_Escape) {
          gRunning = false;
        }
      }

      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }
    }

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);
    usleep(10000);
  }

  XCloseDisplay(display);

  return 0;
}