#include "ED_base.h"
#include "ED_core.h"

#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

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

void DrawAQuad() {
  glClearColor(1.0, 1.0, 1.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-1., 1., -1., 1., 1., 20.);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0., 0., 10., 0., 0., 0., 0., 1., 0.);

  glBegin(GL_QUADS);
  glColor3f(1., 0., 0.);
  glVertex3f(-.75, -.75, 0.);
  glColor3f(0., 1., 0.);
  glVertex3f(.75, -.75, 0.);
  glColor3f(0., 0., 1.);
  glVertex3f(.75, .75, 0.);
  glColor3f(1., 1., 0.);
  glVertex3f(-.75, .75, 0.);
  glEnd();
}

int main(int argc, char const *argv[]) {
  Display *display;
  Window root_window;
  Window window;
  int screen;

  display = XOpenDisplay(0);
  if (display == 0) {
    fprintf(stderr, "Cannot open display\n");
    return 1;
  }

  screen = DefaultScreen(display);
  root_window = RootWindow(display, screen);
  u32 border_color = WhitePixel(display, screen);
  u32 bg_color = BlackPixel(display, screen);

  // Choose visual
  GLint attrs[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
  XVisualInfo *visual = glXChooseVisual(display, 0, attrs);
  if (visual == NULL) {
    printf("No appropriate visual found\n");
    exit(1);
  }

  Colormap colormap =
      XCreateColormap(display, root_window, visual->visual, AllocNone);
  XSetWindowAttributes window_attrs;
  window_attrs.colormap = colormap;
  window_attrs.event_mask = (ExposureMask | KeyPressMask | KeyReleaseMask |
                             ButtonPressMask | StructureNotifyMask);

  window = XCreateWindow(
      display, root_window, 0, 0, kWindowWidth, kWindowHeight, 0, visual->depth,
      InputOutput, visual->visual, CWColormap | CWEventMask, &window_attrs);

  XMapWindow(display, window);
  XStoreName(display, window, "Editor");

  GLXContext glc = glXCreateContext(display, visual, NULL, GL_TRUE);
  glXMakeCurrent(display, window, glc);

  glEnable(GL_DEPTH_TEST);

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

    g_pixel_buffer.memory = (void *)gXImage->data;
    g_pixel_buffer.width = kWindowWidth;
    g_pixel_buffer.height = kWindowHeight - 1;
    g_pixel_buffer.max_width = kWindowWidth;
    g_pixel_buffer.max_height = kWindowHeight;

    gc = XCreateGC(display, window, 0, &gcvalues);
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

  bool redraw = false;

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

      if (event.type == Expose) {
        redraw = true;
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

    if (redraw) {
      XWindowAttributes gwa;
      XGetWindowAttributes(display, window, &gwa);
      glViewport(0, 0, gwa.width, gwa.height);
      DrawAQuad();
      glXSwapBuffers(display, window);
      printf("redraw\n");
      redraw = false;
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

    // XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
    //           kWindowHeight);

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
