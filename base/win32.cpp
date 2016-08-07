#include <stdlib.h>
#include <time.h>

#include "base.h"
#include "core.h"

#include <windows.h>
#include <windowsX.h>
#include <intrin.h>
#include <gl/gl.h>

global bool gRunning;

global pixel_buffer gPixelBuffer;
global GLuint gTextureHandle;

static void Win32UpdateWindow(HDC hdc) {
  if (!gPixelBuffer.memory) return;

  glViewport(0, 0, gPixelBuffer.width, gPixelBuffer.height);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, gTextureHandle);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, gPixelBuffer.width,
               gPixelBuffer.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
               gPixelBuffer.memory);

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
  if (!gPixelBuffer.memory) return;  // no buffer yet

  RECT client_rect;
  GetClientRect(window, &client_rect);
  int width = client_rect.right - client_rect.left;
  int height = client_rect.bottom - client_rect.top;

  if (width > gPixelBuffer.max_width) {
    width = gPixelBuffer.max_width;
  }
  if (height > gPixelBuffer.max_height) {
    height = gPixelBuffer.max_height;
  }

  gPixelBuffer.width = width;
  gPixelBuffer.height = height;
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
      Assert(!"Keyboard input came in through a non-dispatch message!");
    } break;

    default: { Result = DefWindowProc(Window, uMsg, wParam, lParam); } break;
  }

  return Result;
}

inline v2 Win32GetCursorPosition(MSG Message) {
  return V2i(GET_X_LPARAM(Message.lParam),
             gPixelBuffer.height - GET_Y_LPARAM(Message.lParam));
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

  if (RegisterClass(&WindowClass)) {
    // Create window so that its client area is exactly kWindowWidth/Height
    DWORD WindowStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
    RECT WindowRect = {};

    // TODO: get monitor size
    const int kWindowWidth = 1500;
    const int kWindowHeight = 1000;

    WindowRect.right = kWindowWidth;
    WindowRect.bottom = kWindowHeight;
    AdjustWindowRect(&WindowRect, WindowStyle, 0);
    int WindowWidth = WindowRect.right - WindowRect.left;
    int WindowHeight = WindowRect.bottom - WindowRect.top;
    HWND Window = CreateWindow(WindowClass.lpszClassName, 0, WindowStyle,
                               CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth,
                               WindowHeight, 0, 0, hInstance, 0);

    if (Window) {
      // We're not going to release it as we use CS_OWNDC
      HDC hdc = GetDC(Window);

      gRunning = true;

      // Init pixel buffer
      gPixelBuffer.max_width = 3000;
      gPixelBuffer.max_height = 3000;
      gPixelBuffer.memory = VirtualAlloc(
          0, gPixelBuffer.max_width * gPixelBuffer.max_height * sizeof(u32),
          MEM_COMMIT, PAGE_READWRITE);

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

        int SuggestedPixelFormatIndex =
            ChoosePixelFormat(hdc, &DesiredPixelFormat);
        PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
        DescribePixelFormat(hdc, SuggestedPixelFormatIndex,
                            sizeof(SuggestedPixelFormat),
                            &SuggestedPixelFormat);

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
            Assert(false);
          }
        } else {
          // Something's wrong
          Assert(false);
        }
      }

      user_input input = {};
      input.base = V2i(300, 300);

      // Event loop
      while (gRunning) {
        // Process messages
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
          // Get keyboard messages
          switch (Message.message) {
            case WM_QUIT: {
              gRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
              u32 VKCode = (u32)Message.wParam;
              bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
              bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);

              bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
              if ((VKCode == VK_F4) && AltKeyWasDown) {
                gRunning = false;
              }
              if (VKCode == VK_ESCAPE) {
                gRunning = false;
              }
            } break;

            case WM_LBUTTONDOWN: {
              v2 position = Win32GetCursorPosition(Message);
              input.angle = AngleBetween(position - input.base, V2i(1, 0));
              input.pointer = position;
            } break;

            case WM_RBUTTONDOWN: {
              v2 position = Win32GetCursorPosition(Message);
              input.base = position;
            } break;

            case WM_MOUSEMOVE: {
              bool LeftButtonIsDown = ((Message.wParam & MK_LBUTTON) != 0);
              v2 position = Win32GetCursorPosition(Message);
              if (LeftButtonIsDown && position != input.base) {
                input.angle = AngleBetween(position - input.base, V2i(1, 0));
                input.pointer = position;
              }
            } break;

            case WM_MOUSEWHEEL: {
              v2 position = V2i(GET_X_LPARAM(Message.lParam),
                                GET_Y_LPARAM(Message.lParam));
              int delta = GET_WHEEL_DELTA_WPARAM(Message.wParam) / WHEEL_DELTA;
            } break;

            default: {
              TranslateMessage(&Message);
              DispatchMessageA(&Message);
            } break;
          }
        }

        UpdateAndRender(&gPixelBuffer, input);

        Win32UpdateWindow(hdc);
      }
    }
  } else {
    // TODO: logging
    OutputDebugStringA("Couldn't register window class");
  }

  return 0;
}