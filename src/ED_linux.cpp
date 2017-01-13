// ============================ Program code ==================================

#include <x86intrin.h>  // __rdtsc()

#include "ED_base.h"
#include "ED_math.h"
#include "ED_core.h"
#include "ED_model.h"
#include "editors/editors.h"
#include "ui/ED_ui.h"
#include "debug/ED_debug.h"

#include "ED_core.cpp"
#include "ED_math.cpp"
#include "ED_model.cpp"
#include "ED_drawing.cpp"
#include "editors/3dview.cpp"
#include "ui/ED_ui.cpp"

// ========================== Platform headers ================================

#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include <GL/gl.h>
#include <GL/glx.h>

// ============================== OpenGL ======================================

global GLuint g_texture_handle;

// GLX code taken from (and modified later)
// https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig,
                                                     GLXContext, Bool,
                                                     const int *);

typedef void (*glXSwapIntervalEXTProc)(Display *, GLXDrawable, int);

global bool ctxErrorOccurred = false;
int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
  ctxErrorOccurred = true;
  return 0;
}

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
bool linux_is_glx_extension_supported(const char *extList,
                                      const char *extension) {
  const char *start;
  const char *where, *terminator;

  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if (where || *extension == '\0') return false;

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for (start = extList;;) {
    where = strstr(start, extension);
    if (!where) break;
    terminator = where + strlen(extension);
    if (where == start || *(where - 1) == ' ')
      if (*terminator == ' ' || *terminator == '\0') return true;
    start = terminator;
  }

  return false;
}

Window linux_create_opengl_window(Display *display, int width, int height) {
  Window window;

  // Check GLX version
  // FBConfigs were added in GLX version 1.3
  int glx_major, glx_minor;
  if (!glXQueryVersion(display, &glx_major, &glx_minor) ||
      ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1)) {
    printf("Invalid GLX version\n");
    exit(1);
  }

  // Get a matching FB config
  int visual_attribs[] = {
      GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
      GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
      GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8,
      GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, GLX_DOUBLEBUFFER, True,
      // GLX_SAMPLE_BUFFERS  , 1,
      // GLX_SAMPLES         , 4,
      None};

  // Getting matching framebuffer configs
  int fbcount;
  GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display),
                                       visual_attribs, &fbcount);
  if (!fbc) {
    printf("Failed to retrieve a framebuffer config\n");
    exit(1);
  }
  // printf("Found %d matching FB configs.\n", fbcount);

  // Pick the FB config/visual with the most samples per pixel
  // printf("Getting XVisualInfos\n");
  int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

  for (int i = 0; i < fbcount; ++i) {
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbc[i]);
    if (vi) {
      int samp_buf, samples;
      glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
      glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);

      // printf(
      //     "  Matching fbconfig %d, visual ID 0x%2lx: SAMPLE_BUFFERS = %d,"
      //     " SAMPLES = %d\n",
      //     i, vi->visualid, samp_buf, samples);

      if (best_fbc < 0 || (samp_buf && samples > best_num_samp))
        best_fbc = i, best_num_samp = samples;
      if (worst_fbc < 0 || !samp_buf || samples < worst_num_samp)
        worst_fbc = i, worst_num_samp = samples;
    }
    XFree(vi);
  }

  GLXFBConfig bestFbc = fbc[best_fbc];

  // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
  XFree(fbc);

  // Get a visual
  XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);
  // printf("Chosen visual ID = 0x%lx\n", vi->visualid);

  // Create colormap
  XSetWindowAttributes swa;
  Colormap cmap;
  swa.colormap = cmap = XCreateColormap(
      display, RootWindow(display, vi->screen), vi->visual, AllocNone);
  swa.background_pixmap = None;
  swa.border_pixel = 0;
  swa.event_mask = StructureNotifyMask;

  // Create window
  window = XCreateWindow(display, RootWindow(display, vi->screen), 0, 0, width,
                         height, 0, vi->depth, InputOutput, vi->visual,
                         CWBorderPixel | CWColormap | CWEventMask, &swa);
  if (!window) {
    printf("Failed to create window.\n");
    exit(1);
  }

  // Done with the visual info data
  XFree(vi);

  XStoreName(display, window, "Editor");

  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | StructureNotifyMask);

  // printf("TODO: mouse move events\n");

  XMapRaised(display, window);

  // Get the default screen's GLX extension list
  const char *glxExts =
      glXQueryExtensionsString(display, DefaultScreen(display));

  // NOTE: It is not necessary to create or make current to a context before
  // calling glXGetProcAddressARB
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB =
      (glXCreateContextAttribsARBProc)glXGetProcAddressARB(
          (const GLubyte *)"glXCreateContextAttribsARB");

  GLXContext ctx = 0;

  // Install an X error handler so the application won't exit if GL 3.0
  // context allocation fails.
  //
  // Note this error handler is global.  All display connections in all threads
  // of a process use the same error handler, so be sure to guard against other
  // threads issuing X commands while this code is running.
  ctxErrorOccurred = false;
  int (*oldHandler)(Display *, XErrorEvent *) =
      XSetErrorHandler(&ctxErrorHandler);

  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if (!linux_is_glx_extension_supported(glxExts, "GLX_ARB_create_context") ||
      !glXCreateContextAttribsARB) {
    printf(
        "glXCreateContextAttribsARB() not found"
        " ... using old-style GLX context\n");
    ctx = glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
  } else {
    int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                             GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                             // GLX_CONTEXT_FLAGS_ARB        ,
                             // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                             None};

    // printf("Creating context\n");
    ctx =
        glXCreateContextAttribsARB(display, bestFbc, 0, True, context_attribs);

    // Sync to ensure any errors generated are processed.
    XSync(display, False);
    if (!ctxErrorOccurred && ctx) {
      // printf("Created GL 3.0 context\n");
    } else {
      // Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
      // When a context version below 3.0 is requested, implementations will
      // return the newest context version compatible with OpenGL versions less
      // than version 3.0.
      // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
      context_attribs[1] = 1;
      // GLX_CONTEXT_MINOR_VERSION_ARB = 0
      context_attribs[3] = 0;

      ctxErrorOccurred = false;

      printf(
          "Failed to create GL 3.0 context"
          " ... using old-style GLX context\n");
      ctx = glXCreateContextAttribsARB(display, bestFbc, 0, True,
                                       context_attribs);
    }
  }

  // Sync to ensure any errors generated are processed.
  XSync(display, False);

  // Restore the original error handler
  XSetErrorHandler(oldHandler);

  if (ctxErrorOccurred || !ctx) {
    printf("Failed to create an OpenGL context\n");
    exit(1);
  }

  // Verifying that context is a direct context
  if (!glXIsDirect(display, ctx)) {
    // printf("Indirect GLX rendering context obtained\n");
  } else {
    // printf("Direct GLX rendering context obtained\n");
  }

  bool ctx_enabled = glXMakeCurrent(display, window, ctx);
  if (!ctx_enabled) {
    printf("Couldn't make GLX context current\n");
    exit(1);
  }

  glGenTextures(1, &g_texture_handle);

  glXSwapIntervalEXTProc glXSwapIntervalEXT =
      (glXSwapIntervalEXTProc)glXGetProcAddress(
          (const GLubyte *)"glXSwapIntervalEXT");
  if (!glXSwapIntervalEXT) {
    // Fail for now
    printf("No vsync available. Exiting.\n");
    exit(1);
  }
  glXSwapIntervalEXT(display, window, 0);

  return window;
}

// =========================== Platform code ==================================

global timespec g_timestamp;
global XImage *g_ximage;

u64 linux_time_elapsed() {
  // Assumes g_timestamp has been set
  u64 result;
  timespec now, result_timespec;
  clock_gettime(CLOCK_MONOTONIC, &now);
  if ((now.tv_nsec - g_timestamp.tv_nsec) < 0) {
    result_timespec.tv_sec = now.tv_sec - g_timestamp.tv_sec - 1;
    result_timespec.tv_nsec = 1000000000 + now.tv_nsec - g_timestamp.tv_nsec;
  } else {
    result_timespec.tv_sec = now.tv_sec - g_timestamp.tv_sec;
    result_timespec.tv_nsec = now.tv_nsec - g_timestamp.tv_nsec;
  }
  result = result_timespec.tv_nsec;
  g_timestamp = now;
  return result;
}

int main(int argc, char *argv[]) {
  // Allocate main memory
  g_program_memory.start = malloc(MAX_INTERNAL_MEMORY_SIZE);
  g_program_memory.free_memory = g_program_memory.start;

  // Main program state - note that window size is set there
  Program_State *state =
      (Program_State *)g_program_memory.allocate(sizeof(Program_State));
  state->init(&g_program_memory, &g_pixel_buffer);

  Display *display;
  Window window;

  // Open display
  display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Failed to open X display\n");
    exit(1);
  }

#if ED_LINUX_OPENGL
  window = linux_create_opengl_window(display, state->kWindowWidth,
                                      state->kWindowHeight);
#else
  int screen = DefaultScreen(display);

  window = XCreateSimpleWindow(display, RootWindow(display, screen), 300, 300,
                               state->kWindowWidth, state->kWindowHeight, 0,
                               WhitePixel(display, screen),
                               BlackPixel(display, screen));

  XSetStandardProperties(display, window, "Editor", "Hi!", None, NULL, 0, NULL);

  XSelectInput(display, window, ExposureMask | KeyPressMask | KeyReleaseMask |
                                    ButtonPressMask | StructureNotifyMask);
  XMapRaised(display, window);

  GC gc;
  XGCValues gcvalues;

  // Create x image
  {
    for (;;) {
      XEvent e;
      XNextEvent(display, &e);
      if (e.type == MapNotify) break;
    }

    g_ximage = XGetImage(display, window, 0, 0, state->kWindowWidth,
                         state->kWindowHeight, AllPlanes, ZPixmap);

    free(g_pixel_buffer.memory);
    g_pixel_buffer.memory = (void *)g_ximage->data;
    g_pixel_buffer.width = state->kWindowWidth;
    g_pixel_buffer.height = state->kWindowHeight;

    gc = XCreateGC(display, window, 0, &gcvalues);
  }
#endif

  Atom wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteMessage, 1);

  // Define cursors
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

  g_running = true;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &g_timestamp);

  while (g_running) {
    // Process events
    while (XPending(display)) {
      XEvent event;
      XNextEvent(display, &event);

      if (event.type == KeyPress || event.type == KeyRelease) {
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
            g_running = false;
          }
          if (key == XK_Up) {
            new_input->buttons[IB_up] = pressed;
          }
          if (key == XK_Down) {
            new_input->buttons[IB_down] = pressed;
          }
          if (key == XK_Left) {
            new_input->buttons[IB_left] = pressed;
          }
          if (key == XK_Right) {
            new_input->buttons[IB_right] = pressed;
          }
          if (key == XK_Shift_L || key == XK_Shift_R) {
            new_input->buttons[IB_shift] = pressed;
          }
          if (('a' <= symbol && symbol <= 'z') ||
              ('A' <= symbol && symbol <= 'Z') ||
              ('0' <= symbol && symbol <= '9')) {
            // Convert small letters to capitals
            if ('a' <= symbol && symbol <= 'z') {
              symbol += ('A' - 'a');
            }
            new_input->buttons[IB_key] = pressed;
            new_input->symbol = symbol;
          }
        }
      }

      if (event.type == ButtonPress) {
        switch (event.xbutton.button) {
          case Button4: {
            new_input->scroll++;
          } break;
          case Button5: {
            new_input->scroll--;
          } break;
        }
      }

      // Close window message
      if (event.type == ClientMessage) {
        if ((unsigned)event.xclient.data.l[0] == wmDeleteMessage) {
          g_running = false;
        }
      }
    }

    {
      // Get mouse position
      int root_x, root_y;
      unsigned int mouse_mask;
      Window root_return, child_return;
      XQueryPointer(display, window, &root_return, &child_return, &root_x,
                    &root_y, &new_input->mouse.x, &new_input->mouse.y,
                    &mouse_mask);
      new_input->buttons[IB_mouse_left] = mouse_mask & Button1Mask;
      new_input->buttons[IB_mouse_middle] = mouse_mask & Button2Mask;
      new_input->buttons[IB_mouse_right] = mouse_mask & Button3Mask;
    }

    Update_Result result;
    {
      TIMED_BLOCK();
      result = update_and_render(&g_program_memory, state, new_input);
    }

    #include "debug/ED_debug_draw.cpp"

    assert(0 <= result.cursor && result.cursor < Cursor_Type__COUNT);
    XDefineCursor(display, window, linux_cursors[result.cursor]);

#if ED_LINUX_OPENGL
    {
      glViewport(0, 0, g_pixel_buffer.width, g_pixel_buffer.height);

      glEnable(GL_TEXTURE_2D);

      glBindTexture(GL_TEXTURE_2D, g_texture_handle);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_pixel_buffer.width,
                   g_pixel_buffer.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                   g_pixel_buffer.memory);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      glClearColor(1, 0.5, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);

      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();

      glBegin(GL_TRIANGLES);

      glTexCoord2f(0.0f, 1.0f);
      glVertex2i(-1, -1);
      glTexCoord2f(1.0f, 1.0f);
      glVertex2i(1, -1);
      glTexCoord2f(1.0f, 0.0f);
      glVertex2i(1, 1);

      glTexCoord2f(0.0f, 1.0f);
      glVertex2i(-1, -1);
      glTexCoord2f(0.0f, 0.0f);
      glVertex2i(-1, 1);
      glTexCoord2f(1.0f, 0.0f);
      glVertex2i(1, 1);

      glEnd();

      glXSwapBuffers(display, window);
    }
#else
    XPutImage(display, window, gc, g_ximage, 0, 0, 0, 0, state->kWindowWidth,
              state->kWindowHeight);
#endif  // ED_LINUX_OPENGL

    u64 ns_elapsed = linux_time_elapsed();
    g_FPS.value = (int)(1.0e9 / ns_elapsed);

    // printf("fps: %d, ns: %lu\n", g_FPS, ns_elapsed);

    // Swap inputs
    User_Input *tmp = old_input;
    old_input = new_input;
    new_input = tmp;

    // Zero input
    *new_input = {};
    new_input->old = old_input;  // Save so we can refer to it later

    // Retain the button state
    for (size_t i = 0; i < COUNT_OF(new_input->buttons); i++) {
      new_input->buttons[i] = old_input->buttons[i];
    }
    for (size_t i = 0; i < COUNT_OF(new_input->mouse_positions); i++) {
      new_input->mouse_positions[i] = old_input->mouse_positions[i];
    }
    new_input->symbol = old_input->symbol;
  }

  glXMakeCurrent(display, 0, 0);
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#if ED_LEAKCHECK
  // Only freeing everything for a leak check
  printf("======================= freeing ==========================\n");
  // Free models
  for (int i = 0; i < sb_count(state->models); ++i) {
    state->models[i].destroy();
  }
  sb_free(state->models);

  // Free areas
  for (int i = 0; i < state->UI->num_areas; ++i) {
    free(state->UI->areas[i]);
  }
  sb_free(state->UI->areas);

  // Free splitters
  for (int i = 0; i < state->UI->num_splitters; ++i) {
    free(state->UI->splitters[i]);
  }
  sb_free(state->UI->splitters);

  // Free general stuff
  free(state->UI->z_buffer);
  free(state->UI);
  free(g_font.ttf_raw_data);
  free(g_font.bitmap);
#if ED_LINUX_OPENGL
  free(g_pixel_buffer.memory);
#endif
  free(g_program_memory.start);

  printf("====================== leak check ========================\n");
  stb_leakcheck_dumpmem();
#endif  // ED_LEAKCHECK

  return 0;
}

int g_num_perf_counters = __COUNTER__;
ED_Perf_Counter g_performance_counters[__COUNTER__];

