#include <stdlib.h>
#include <time.h>

#include "base.h"

global void *gWindowsBitmapMemory;

#include <windows.h>
#include <intrin.h>

global BITMAPINFO GlobalBitmapInfo;

global const int kWindowWidth = 1024;
global const int kWindowHeight = 768;

global bool gRunning;

struct point {
  int x;
  int y;
  int z;
};

static void Win32UpdateWindow(HDC hdc) {
  if (!gWindowsBitmapMemory) return;

  StretchDIBits(hdc, 0, 0, kWindowWidth, kWindowHeight,  // dest
                0, 0, kWindowWidth, kWindowHeight,       // src
                gWindowsBitmapMemory, &GlobalBitmapInfo, DIB_RGB_COLORS,
                SRCCOPY);
}

LRESULT CALLBACK
Win32WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  LRESULT Result = 0;

  switch (uMsg) {
    case WM_CLOSE: {
      gRunning = false;
    } break;

    case WM_PAINT: {
      PAINTSTRUCT Paint = {};
      HDC hdc = BeginPaint(hwnd, &Paint);
      Win32UpdateWindow(hdc);
      EndPaint(hwnd, &Paint);
    } break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
      Assert(!"Keyboard input came in through a non-dispatch message!");
    } break;

    default: { Result = DefWindowProc(hwnd, uMsg, wParam, lParam); } break;
  }

  return Result;
}

void DrawPixel(int x, int y, u32 Color) {
  u32 *Pixel = (u32 *)gWindowsBitmapMemory + x + kWindowWidth * y;
  *Pixel = Color;
}

void DrawStar(point *star) {
  DrawPixel(star->x / star->z, star->y / star->z, 0x00FFFFFF);
}

void EraseStar(point *star) {
  DrawPixel(star->x / star->z, star->y / star->z, 0x00000000);
}

void InitStar(point *star) {
  star->x = rand() % kWindowWidth;
  star->y = rand() % kWindowHeight;
  star->z = rand() % 100 + 1;
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
    DWORD WindowStyle = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME | WS_VISIBLE;
    RECT WindowRect = {};
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

      // Init memory
      gWindowsBitmapMemory =
          VirtualAlloc(0, kWindowWidth * kWindowHeight * sizeof(u32),
                       MEM_COMMIT, PAGE_READWRITE);

      // Init bitmap
      GlobalBitmapInfo.bmiHeader.biWidth = kWindowWidth;
      GlobalBitmapInfo.bmiHeader.biHeight = -kWindowHeight;
      GlobalBitmapInfo.bmiHeader.biSize = sizeof(GlobalBitmapInfo.bmiHeader);
      GlobalBitmapInfo.bmiHeader.biPlanes = 1;
      GlobalBitmapInfo.bmiHeader.biBitCount = 32;
      GlobalBitmapInfo.bmiHeader.biCompression = BI_RGB;

      srand((uint)time(NULL));
      const int num_stars = 1000;
      point stars[num_stars];
      for (int i = 0; i < num_stars; i++) {
        InitStar(&stars[i]);
      }

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

            default: {
              TranslateMessage(&Message);
              DispatchMessageA(&Message);
            } break;
          }
        }

        // Draw stars
        for (int i = 0; i < num_stars; i++) {
          point *star = &stars[i];

          EraseStar(star);

          star->z--;
          if (star->z <= 1) {
            InitStar(star);
          }

          DrawStar(star);
        }

        // TODO: sleep on vblank
        Win32UpdateWindow(hdc);
        Sleep(10);
      }
    }
  } else {
    // TODO: logging
    OutputDebugStringA("Couldn't register window class");
  }

  return 0;
}