#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "ED_base.h"
#include "ED_core.h"

#include <intrin.h>
#include <windows.h>
#include <windowsX.h>
#include <gl/gl.h>

global bool gRunning;
global LARGE_INTEGER gPerformanceFrequency;

global Pixel_Buffer g_pixel_buffer;
global void *g_program_memory;
global GLuint gTextureHandle;

static void Win32UpdateWindow(HDC hdc) {
  if (!g_pixel_buffer.memory) return;

  glViewport(0, 0, g_pixel_buffer.width, g_pixel_buffer.height);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, gTextureHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, g_pixel_buffer.width,
               g_pixel_buffer.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
               g_pixel_buffer.memory);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
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

  SwapBuffers(hdc);
}

static void Win32ResizeClientWindow(HWND window) {
  if (!g_pixel_buffer.memory) return;  // no buffer yet

  RECT client_rect;
  GetClientRect(window, &client_rect);
  int width = client_rect.right - client_rect.left;
  int height = client_rect.bottom - client_rect.top;

  if (width > g_pixel_buffer.max_width) {
    width = g_pixel_buffer.max_width;
  }
  if (height > g_pixel_buffer.max_height) {
    height = g_pixel_buffer.max_height;
  }

  if (!g_pixel_buffer.was_resized) {
    g_pixel_buffer.prev_width = g_pixel_buffer.width;
    g_pixel_buffer.prev_height = g_pixel_buffer.height;
  }
  g_pixel_buffer.width = width;
  g_pixel_buffer.height = height;
  g_pixel_buffer.was_resized = true;
}

LRESULT CALLBACK
Win32WindowProc(HWND Window, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  LRESULT Result = 0;

  switch (uMsg) {
    case WM_SIZE: {
      Win32ResizeClientWindow(Window);
    } break;

    case WM_CLOSE: {
      gRunning = false;
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint = {};
      HDC hdc = BeginPaint(Window, &Paint);
      Win32UpdateWindow(hdc);
      EndPaint(Window, &Paint);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      assert(!"Keyboard input came in through a non-dispatch message!");
    } break;

    default: { Result = DefWindowProc(Window, uMsg, wParam, lParam); } break;
  }

  return Result;
}

inline LARGE_INTEGER Win32GetWallClock() {
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);

  return Result;
}

inline r32 Win32GetMillisecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
  r32 Result = 1000.0f * (r32)(End.QuadPart - Start.QuadPart) /
               (r32)gPerformanceFrequency.QuadPart;

  return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow) {
  WNDCLASS WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
  WindowClass.lpfnWndProc = Win32WindowProc;
  WindowClass.hInstance = hInstance;
  WindowClass.lpszClassName = "VMWindowClass";

  // Set target sleep resolution
  {
    TIMECAPS tc;
    UINT wTimerRes;

    if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) {
      OutputDebugStringA("Cannot set the sleep resolution\n");
      exit(1);
    }

    wTimerRes = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);  // 1 ms
    timeBeginPeriod(wTimerRes);
  }

  QueryPerformanceFrequency(&gPerformanceFrequency);

  if (!RegisterClass(&WindowClass)) {
    // TODO: logging
    printf("Couldn't register window class\n");
    exit(1);
  }

  // Create window so that its client area is exactly kWindowWidth/Height
  DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  RECT WindowRect = {};

  // TODO: get monitor size
  const int kWindowWidth = 1000;
  const int kWindowHeight = 700;

  WindowRect.right = kWindowWidth;
  WindowRect.bottom = kWindowHeight;
  AdjustWindowRect(&WindowRect, WindowStyle, 0);
  int WindowWidth = WindowRect.right - WindowRect.left;
  int WindowHeight = WindowRect.bottom - WindowRect.top;
  HWND Window = CreateWindow(WindowClass.lpszClassName, 0, WindowStyle,
                             CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth,
                             WindowHeight, 0, 0, hInstance, 0);

  if (!Window) {
    printf("Couldn't create window\n");
    exit(1);
  }

  // We're not going to release it as we use CS_OWNDC
  HDC hdc = GetDC(Window);

  gRunning = true;

  // Allocate program memory
  g_program_memory = malloc(MAX_INTERNAL_MEMORY_SIZE);
  // TODO: add checks for overflow when allocating

  // Init pixel buffer
  g_pixel_buffer.max_width = 3000;
  g_pixel_buffer.max_height = 3000;
  g_pixel_buffer.memory = malloc(g_pixel_buffer.max_width *
                                 g_pixel_buffer.max_height * sizeof(u32));

  // Set proper buffer values based on actual client size
  Win32ResizeClientWindow(Window);

  // Init OpenGL
  {
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    DesiredPixelFormat.dwFlags =
        PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;

    int SuggestedPixelFormatIndex = ChoosePixelFormat(hdc, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(hdc, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);

    SetPixelFormat(hdc, SuggestedPixelFormatIndex, &SuggestedPixelFormat);

    HGLRC OpenGLRC = wglCreateContext(hdc);
    if (wglMakeCurrent(hdc, OpenGLRC)) {
      // Success
      glGenTextures(1, &gTextureHandle);

      typedef BOOL WINAPI wgl_swap_interval_ext(int interval);
      wgl_swap_interval_ext *wglSwapInterval =
          (wgl_swap_interval_ext *)wglGetProcAddress("wglSwapIntervalEXT");
      if (wglSwapInterval) {
        wglSwapInterval(1);
      } else {
        // VSync not enabled or not supported
        assert(false);
      }
    } else {
      // Something's wrong
      assert(false);
    }
  }

  Cursor_Type current_cursor = Cursor_Type_Arrow;
  HCURSOR win_cursors[Cursor_Type__COUNT];
  win_cursors[Cursor_Type_Arrow] = LoadCursor(NULL, IDC_ARROW);
  win_cursors[Cursor_Type_Cross] = LoadCursor(NULL, IDC_CROSS);
  win_cursors[Cursor_Type_Hand] = LoadCursor(NULL, IDC_HAND);
  win_cursors[Cursor_Type_Resize_Vert] = LoadCursor(NULL, IDC_SIZENS);
  win_cursors[Cursor_Type_Resize_Horiz] = LoadCursor(NULL, IDC_SIZEWE);

  User_Input inputs[2];
  User_Input *old_input = &inputs[0];
  User_Input *new_input = &inputs[1];
  *new_input = {};

  assert(&new_input->terminator - &new_input->buttons[0] <
         COUNT_OF(new_input->buttons));

  LARGE_INTEGER last_timestamp = Win32GetWallClock();

  // Event loop
  while (gRunning) {
    // Process messages
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
      // Get keyboard messages
      switch (message.message) {
        case WM_QUIT: {
          gRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
          u32 vk_code = (u32)message.wParam;
          b32 was_down = ((message.lParam & (1 << 30)) != 0);
          b32 is_down = ((message.lParam & (1 << 31)) == 0);

          b32 alt_key_was_down = (message.lParam & (1 << 29));
          if ((vk_code == VK_F4) && alt_key_was_down) {
            gRunning = false;
          }
          if (was_down == is_down) {
            break;  // nothing has changed
          }
          if (vk_code == VK_ESCAPE) {
            gRunning = false;
          }
          if (vk_code == VK_UP || vk_code == 'W') {
            new_input->up = is_down;
          }
          if (vk_code == VK_DOWN || vk_code == 'S') {
            new_input->down = is_down;
          }
          if (vk_code == VK_LEFT || vk_code == 'A') {
            new_input->left = is_down;
          }
          if (vk_code == VK_RIGHT || vk_code == 'D') {
            new_input->right = is_down;
          }
        } break;

        default: {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        } break;
      }
    }

    // Get mouse input
    {
      POINT mouse_pointer;
      GetCursorPos(&mouse_pointer);
      ScreenToClient(Window, &mouse_pointer);

      new_input->mouse = {mouse_pointer.x, mouse_pointer.y};

      new_input->mouse_left = GetKeyState(VK_LBUTTON) & (1 << 15);
      new_input->mouse_right = GetKeyState(VK_RBUTTON) & (1 << 15);
      new_input->mouse_middle = GetKeyState(VK_MBUTTON) & (1 << 15);
    }

    Update_Result result =
        update_and_render(g_program_memory, &g_pixel_buffer, new_input);

    assert(0 <= result.cursor && result.cursor < Cursor_Type__COUNT);
    SetCursor(win_cursors[result.cursor]);

    Win32UpdateWindow(hdc);

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

#if 0
    r32 ms_elapsed =
        Win32GetMillisecondsElapsed(last_timestamp, Win32GetWallClock());
    printf("%.2f - ", ms_elapsed);
    last_timestamp = Win32GetWallClock();
#endif
  }

  return 0;
}
