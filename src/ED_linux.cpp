#include "ED_base.h"
#include "ED_core.h"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
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
global Pixel_Buffer g_pixel_buffer;
global Program_Memory g_program_memory;

global const int kWindowWidth = 1000;
global const int kWindowHeight = 700;

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
  Pixmap pixmap;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }

    // gXImage =
    //     XGetImage(display, window, 0, 0, width, height, AllPlanes, ZPixmap);

    // g_pixel_buffer.memory = (void *)result->data;
    // g_pixel_buffer.width = width;
    // g_pixel_buffer.height = height - 1;
    // g_pixel_buffer.max_width = 3000;
    // g_pixel_buffer.max_height = 3000;

    // assert(width <= g_pixel_buffer.max_width);
    // assert(height <= g_pixel_buffer.max_height);

    // int width = 100;
    // int height = 100;
    // int depth = 32;          // works fine with depth = 24
    // int bitmap_pad = 32;     // 32 for 24 and 32 bpp, 16, for 15&16
    // int bytes_per_line = 0;  // number of bytes in the client image between
    // the
    //                          // start of one scanline and the start of the
    //                          next
    // Display *display = XOpenDisplay(0);
    // unsigned char *image32 = (unsigned char *)malloc(width * height * 4);
    // XImage *img =
    //     XCreateImage(display, CopyFromParent, depth, ZPixmap, 0, image32,
    //     width,
    //                  height, bitmap_pad, bytes_per_line);
    // Pixmap p = XCreatePixmap(display, XDefaultRootWindow(display), width,
    //                          height, depth);
    // XGCValues gcvalues;
    // GC gc = XCreateGC(display, p, 0, &gcvalues);
    // XPutImage(display, p, gc, img, 0, 0, 0, 0, width,
    //           height);  // 0, 0, 0, 0 are src x,y and dst x,y

    g_pixel_buffer.width = kWindowWidth;
    g_pixel_buffer.height = kWindowHeight - 1;
    g_pixel_buffer.memory = malloc(g_pixel_buffer.max_width *
                                   g_pixel_buffer.max_height * sizeof(u32));
    int width = g_pixel_buffer.max_width = 3000;
    int height = g_pixel_buffer.max_height = 3000;
    int depth = 32;
    int bitmap_pad = 32;
    int bytes_per_line = 0;

    gXImage = XCreateImage(display, CopyFromParent, depth, ZPixmap, 0,
                           (char *)g_pixel_buffer.memory, width, height,
                           bitmap_pad, bytes_per_line);
    pixmap = XCreatePixmap(display, window, width, height, depth);
    gc = XCreateGC(display, pixmap, 0, &gcvalues);
  }

  // Allocate main memory
  g_program_memory.free_memory = malloc(MAX_INTERNAL_MEMORY_SIZE);

  // Main program state
  Program_State *state =
      (Program_State *)g_program_memory.allocate(sizeof(Program_State));
  state->init(&g_program_memory);

  // Define cursors
  Cursor_Type current_cursor = Cursor_Type_Arrow;
  Cursor linux_cursors[Cursor_Type__COUNT];
  linux_cursors[Cursor_Type_Arrow] = XCreateFontCursor(display, XC_left_ptr);
  linux_cursors[Cursor_Type_Cross] = XCreateFontCursor(display, XC_cross);
  linux_cursors[Cursor_Type_Hand] = XCreateFontCursor(display, XC_hand1);
  linux_cursors[Cursor_Type_Resize_Vert] =
      XCreateFontCursor(display, XC_sb_v_double_arrow);
  linux_cursors[Cursor_Type_Resize_Horiz] =
      XCreateFontCursor(display, XC_sb_h_double_arrow);

  User_Input inputs[2];
  User_Input *old_input = &inputs[0];
  User_Input *new_input = &inputs[1];
  *new_input = {};

  assert(&new_input->terminator - &new_input->buttons[0] <
         COUNT_OF(new_input->buttons));

  gRunning = true;
  bool resize_window = false;

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

      // Window resizing
      if (event.type == ConfigureNotify) {
        XConfigureEvent xce = event.xconfigure;
        if (xce.width != gXImage->width || xce.height != gXImage->height) {
          // Remember to resize the window
          resize_window = true;
          g_pixel_buffer.width = xce.width;
          g_pixel_buffer.height = xce.height;
          printf("resize event: w %d h %d\n", xce.width, xce.height);
        }
      }

      if (event.type == MapNotify) {
        printf("MapNotify event\n");
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

    if (resize_window) {
      // Actually resize the window
      resize_window = false;
      printf("resize to %d:%d\n", g_pixel_buffer.width, g_pixel_buffer.height);
      g_pixel_buffer.was_resized = true;
    }

    {
      // Get mouse position
      int root_x, root_y;
      unsigned int mouse_mask;
      Window root_return, child_return;
      XQueryPointer(display, window, &root_return, &child_return, &root_x,
                    &root_y, &new_input->mouse.x, &new_input->mouse.y,
                    &mouse_mask);
      new_input->mouse_left = mouse_mask & Button1Mask;
      new_input->mouse_right = mouse_mask & Button2Mask;
      new_input->mouse_middle = mouse_mask & Button3Mask;
    }

    Update_Result result =
        update_and_render(&g_program_memory, state, &g_pixel_buffer, new_input);

    assert(0 <= result.cursor && result.cursor < Cursor_Type__COUNT);
    XDefineCursor(display, window, linux_cursors[result.cursor]);

    XPutImage(display, pixmap, gc, gXImage, 0, 0, 0, 0, gXImage->width,
              gXImage->height);

    // Swap inputs
    User_Input *tmp = old_input;
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
