#include "ED_base.h"
#include "ED_core.h"

#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

global bool gRunning;
global pixel_buffer gPixelBuffer;

global const int kWindowWidth = 400;
global const int kWindowHeight = 300;

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

    gPixelBuffer.memory = (void *)gXImage->data;
    gPixelBuffer.width = kWindowWidth;
    gPixelBuffer.height = kWindowHeight;
    gPixelBuffer.max_width = kWindowWidth;
    gPixelBuffer.max_height = kWindowHeight;

    gc = XCreateGC(display, window, 0, &gcvalues);
  }

  user_input inputs[2];
  user_input *old_input = &inputs[0];
  user_input *new_input = &inputs[1];
  *new_input = {};

  Assert(&new_input->terminator - &new_input->buttons[0] <
         COUNT_OF(new_input->buttons));

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
        pressed = true;
      }

      if (event.type == KeyRelease) {
        if (XEventsQueued(display, QueuedAfterReading)) {
          XEvent nev;
          XPeekEvent(display, &nev);

          if (nev.type == KeyPress && nev.xkey.time == event.xkey.time &&
              nev.xkey.keycode == event.xkey.keycode) {
            // Ignore. Key wasn't actually released
            XNextEvent(display, &event);
            retriggered = true;
          }
        }

        if (!retriggered) {
          released = true;
        }
      }

      if (pressed || released) {
        if (key == XK_Escape) {
          gRunning = false;
        } else if (key == XK_Up) {
          new_input->up = pressed;
        } else if (key == XK_Down) {
          new_input->down = pressed;
        } else if (key == XK_Left) {
          new_input->left = pressed;
        } else if (key == XK_Right) {
          new_input->right = pressed;
        }
      }
      // Close window message
      if (event.type == ClientMessage) {
        if (event.xclient.data.l[0] == wmDeleteMessage) {
          gRunning = false;
        }
      }

    }

    UpdateAndRender(&gPixelBuffer, new_input);

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);

    // Swap inputs
    user_input *tmp = old_input;
    old_input = new_input;
    new_input = tmp;

    // Zero input
    *new_input = {};

    // Retain the button state
    for (int i = 0; i < COUNT_OF(new_input->buttons); i++) {
      new_input->buttons[i] = old_input->buttons[i];
    }

    // TODO: think about something more reliable
    usleep(10000);
  }

  XCloseDisplay(display);

  return 0;
}
