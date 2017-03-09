// ============================ Program code ==================================

#include <x86intrin.h>  // __rdtsc()
#include <pthread.h>
#include <semaphore.h>

#include "ED_base.h"
#include "debug/ED_debug.h"
#include "ED_math.h"
#include "ED_core.h"
#include "ED_model.h"
#include "editors/editors.h"
#include "ui/ED_ui.h"

#include "ED_core.cpp"
#include "ED_math.cpp"
#include "ED_model.cpp"
#include "ED_drawing.cpp"
#include "editors/3dview.cpp"
#include "editors/raytrace.cpp"
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

#include <GLFW/glfw3.h>

#define USE_GLFW 0


// =========================== Platform code ==================================

struct Linux_Raytrace_Work_Queue : Raytrace_Work_Queue {
  sem_t semaphore;

  virtual void add_entry(Raytrace_Work_Entry);
};

void Linux_Raytrace_Work_Queue::add_entry(Raytrace_Work_Entry entry) {
  u32 new_next_entry_to_add =
      (this->next_entry_to_add + 1) % COUNT_OF(this->entries);
  assert(new_next_entry_to_add != this->next_entry_to_do);
  Raytrace_Work_Entry *entry_to_add = this->entries + this->next_entry_to_add;
  *entry_to_add = entry;
  asm volatile("" ::: "memory");
  this->next_entry_to_add = new_next_entry_to_add;
  sem_post(&this->semaphore);
}

global Linux_Raytrace_Work_Queue g_raytrace_queue;
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

void *raytrace_worker_thread(void *arg) {
  thread_info *info = (thread_info *)arg;

  Linux_Raytrace_Work_Queue *queue = &g_raytrace_queue;
  for (;;) {
    u32 original_next_entry_to_do = queue->next_entry_to_do;
    u32 new_next_entry_to_do =
        (original_next_entry_to_do + 1) % COUNT_OF(queue->entries);

    if (original_next_entry_to_do != queue->next_entry_to_add) {
      // There's probably work to do
      u32 index = __sync_val_compare_and_swap(&queue->next_entry_to_do,
                                              original_next_entry_to_do,
                                              new_next_entry_to_do);
      if (index == original_next_entry_to_do) {
        // No other thread has beaten us to it, do the work
        Raytrace_Work_Entry entry = queue->entries[index];
        queue->entries_in_progress[info->thread_num] = index;
        // The line above may cause a machine clear but that's probably OK?
        entry.editor->trace_tile(entry.models, entry.start, entry.end);
        printf("Thread %d did work entry %d\n", info->thread_num, index);
        queue->entries_in_progress[info->thread_num] = -1;
      }
    } else {
      // Sleep
      printf("Thread %d went to sleep\n", info->thread_num);
      sem_wait(&queue->semaphore);
      printf("Thread %d has woken up\n", info->thread_num);
    }
  }
}

int main(int argc, char *argv[]) {
  // Allocate main memory
  g_program_memory.start = malloc(MAX_INTERNAL_MEMORY_SIZE);
  g_program_memory.free_memory = g_program_memory.start;

  // Main program state - note that window size is set there
  Program_State *state =
      (Program_State *)g_program_memory.allocate(sizeof(Program_State));
  state->init(&g_program_memory, &g_pixel_buffer,
              (Raytrace_Work_Queue *)&g_raytrace_queue);

#if USE_GLFW

  GLFWwindow *window;

  if (!glfwInit()) {
    return -1;
  }

  window = glfwCreateWindow(640, 480, "Hey", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);

  while (!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(window);

    glfwPollEvents();
  }

#else
  Display *display;
  Window window;

  // Open display
  display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Failed to open X display\n");
    exit(1);
  }

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

  // Create worked threads
  {
    // Init work queue
    g_raytrace_queue.next_entry_to_add = 0;
    g_raytrace_queue.next_entry_to_do = 0;
    sem_init(&g_raytrace_queue.semaphore, 0, 0);

    for (int i = 0; i < g_kNumThreads; ++i) {
      g_threads[i].thread_num = i;
      pthread_t thread_id;  // we forget it since we don't want to talk about it
                            // (maybe tmp)
      int error = pthread_create(&thread_id, NULL, raytrace_worker_thread,
                                 &g_threads[i]);
      if (error) {
        printf("Can't create thread\n");
        exit(EXIT_FAILURE);
      }
    }
  }

  // Main loop
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
            new_input->buttons[IB_escape] = pressed;
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

#if KSJLAKJSFLKJ
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

#endif // USE_GLFW

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
    if (state->UI->areas[i]->editor_raytrace.backbuffer.memory) {
      free(state->UI->areas[i]->editor_raytrace.backbuffer.memory);
    }
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
  free(g_font.tmp_bitmap);
  free(g_font.ttf_raw_data);
  free(g_font.bitmap);
#if ED_LINUX_OPENGL
  free(g_pixel_buffer.memory);
#endif
  free(g_program_memory.start);

  printf("====================== leak check ========================\n");
  stb_leakcheck_dumpmem();
#endif  // ED_LEAKCHECK

  exit(EXIT_SUCCESS);
}

int g_num_perf_counters = __COUNTER__;
ED_Perf_Counter g_performance_counters[__COUNTER__];
