#include "base.h"

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

/*************** TODO *****************

**************************************/

global bool gRunning;
global void *gLinuxBitmapMemory;

global const int kWindowWidth = 1024;
global const int kWindowHeight = 768;

global XImage *gXImage;


r32 RandomFloat() {
  return (r32)rand()/(r32)RAND_MAX;
}


struct Star {
  static const int kSpeed = 10;
  static const int kDepth = 1000;

  r32 x;
  r32 y;
  r32 z;

  void Init();
  void Erase();
  void Draw();
  void Update();

private:
  void DrawPixel(u32 Color);
};

void Star::DrawPixel(u32 Color) {
  int half_width = kWindowWidth / 2;
  int half_height = kWindowHeight / 2;

  int x = (this->x / this->z) * (r32)half_width + half_width;
  int y = (this->y / this->z) * (r32)half_height + half_height;

  if (x < 0 || x >= kWindowWidth || y < 0 || y >= kWindowHeight) {
    return;
  }

  u32 *pixel = (u32 *)gLinuxBitmapMemory + x + kWindowWidth * y;
  *pixel = Color;
}

void Star::Erase() {
  this->DrawPixel(0x00000000);
}

void Star::Draw() {
  this->DrawPixel(0x00FFFFFF);
}

void Star::Init() {
  this->x = 2 * RandomFloat() - 1;
  this->y = 2 * RandomFloat() - 1;
  this->z = RandomFloat();
}

void Star::Update() {
  if (this->z <= 0) {
    this->Init();
    return;
  }
  this->z -= (r32)kSpeed / (r32)kDepth;
}


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

  srand(time(NULL));
  const int kStarCount = 10000;
  Star stars[kStarCount];
  for (int i = 0; i < kStarCount; i++) {
    stars[i].Init();
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

    for (int i = 0; i < kStarCount; i++) {
      Star *star = &stars[i];
      star->Erase();
      star->Update();
      star->Draw();
    }

    XPutImage(display, window, gc, gXImage, 0, 0, 0, 0, kWindowWidth,
              kWindowHeight);
    usleep(10000);
  }

  XCloseDisplay(display);

  return 0;
}