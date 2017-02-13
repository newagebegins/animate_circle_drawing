#include <windows.h>
#include <stdbool.h>

typedef enum {
  COLOR_BLACK = 0x00000000
} Color;

typedef struct {
  Color *memory;
  size_t size;
  int width;
  int height;
  BITMAPINFO info;
} BackBuffer;

BackBuffer makeBackBuffer(int width, int height) {
  BackBuffer bb = {0};

  bb.size = width * height * sizeof(*bb.memory);
  bb.memory = malloc(bb.size);

  bb.width = width;
  bb.height = height;

  bb.info.bmiHeader.biSize = sizeof(bb.info.bmiHeader);
  bb.info.bmiHeader.biWidth = width;
  bb.info.bmiHeader.biHeight = height;
  bb.info.bmiHeader.biPlanes = 1;
  bb.info.bmiHeader.biBitCount = 32;
  bb.info.bmiHeader.biCompression = BI_RGB;

  return bb;
}

void clearBackBuffer(BackBuffer *bb, Color color) {
  memset(bb->memory, color, bb->size);
}

LRESULT CALLBACK wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch (msg) {
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    default:
      return DefWindowProc(wnd, msg, wparam, lparam);
  }
  return 0;
}

int CALLBACK WinMain(HINSTANCE inst, HINSTANCE prevInst, LPSTR cmdLine, int cmdShow) {
  UNREFERENCED_PARAMETER(prevInst);
  UNREFERENCED_PARAMETER(cmdLine);

  WNDCLASS wndClass = {0};
  wndClass.style = CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc = wndProc;
  wndClass.hInstance = inst;
  wndClass.hCursor = LoadCursor(0, IDC_ARROW);
  wndClass.lpszClassName = "Pinball";
  RegisterClass(&wndClass);

  int windowWidth = 1920/2;
  int windowHeight = 1080/2;

  RECT crect = {0};
  crect.right = windowWidth;
  crect.bottom = windowHeight;

  DWORD wndStyle = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
  AdjustWindowRect(&crect, wndStyle, 0);

  HWND wnd = CreateWindowEx(0, wndClass.lpszClassName, "Pinball", wndStyle, 300, 0,
                            crect.right - crect.left, crect.bottom - crect.top,
                            0, 0, inst, 0);
  ShowWindow(wnd, cmdShow);
  UpdateWindow(wnd);

  float dt = 0.0f;
  LARGE_INTEGER perfcFreq = {0};
  LARGE_INTEGER perfc = {0};
  LARGE_INTEGER prefcPrev = {0};

  QueryPerformanceFrequency(&perfcFreq);
  QueryPerformanceCounter(&perfc);

  HDC hDC = GetDC(wnd);
  BackBuffer bb = makeBackBuffer(windowWidth/2, windowHeight/2);

  bool running = true;

  while (running) {
    prefcPrev = perfc;
    QueryPerformanceCounter(&perfc);
    dt = (float)(perfc.QuadPart - prefcPrev.QuadPart) / (float)perfcFreq.QuadPart;

    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
      switch (msg.message) {
        case WM_QUIT:
          running = false;
          break;

        case WM_KEYDOWN:
        case WM_KEYUP:
          switch (msg.wParam) {
            case VK_ESCAPE:
              running = false;
              break;
          }
          break;

        default:
          TranslateMessage(&msg);
          DispatchMessage(&msg);
          break;
      }
    }

    clearBackBuffer(&bb, COLOR_BLACK);
    StretchDIBits(hDC, 0, 0, windowWidth, windowHeight,
                  0, 0, bb.width, bb.height, bb.memory,
                  &bb.info, DIB_RGB_COLORS, SRCCOPY);
  }
}
