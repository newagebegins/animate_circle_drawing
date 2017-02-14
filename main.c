#include <windows.h>
#include <stdbool.h>

typedef enum {
  COLOR_BLACK   = 0xFF000000,
  COLOR_WHITE   = 0xFFFFFFFF,
  COLOR_GREEN   = 0xFF00FF00,
  COLOR_RED     = 0xFFFF0000,
  COLOR_BLUE    = 0xFF0000FF,
  COLOR_YELLOW  = 0xFFFFFF00,
  COLOR_MAGENTA = 0xFFFF00FF,
  COLOR_CYAN    = 0xFF00FFFF,
  COLOR_PINK    = 0xFFF6A5D1,
} Color;

typedef struct {
  Color *memory;
  size_t size;
  int width;
  int height;
  int windowWidth;
  int windowHeight;
  HDC deviceContext;
  BITMAPINFO info;
} BackBuffer;

BackBuffer makeBackBuffer(HDC deviceContext, int windowWidth, int windowHeight, int width, int height) {
  BackBuffer bb = {0};

  bb.deviceContext = deviceContext;
  bb.windowWidth = windowWidth;
  bb.windowHeight = windowHeight;

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

void displayBackBuffer(BackBuffer *bb) {
  StretchDIBits(bb->deviceContext, 0, 0, bb->windowWidth, bb->windowHeight,
                0, 0, bb->width, bb->height, bb->memory,
                &bb->info, DIB_RGB_COLORS, SRCCOPY);
}

void clearBackBuffer(BackBuffer *bb, Color color) {
  memset(bb->memory, color, bb->size);
}

void setPixel(BackBuffer *bb, int x, int y, Color color) {
  bb->memory[y*bb->width + x] = color;
}

typedef struct {
  int x0;
  int y0;
  int radiusSquared;
  float setPixelTime;
  int x;
  int y;
  float t;
  int currentOctant;
} AnimateCircleState;

AnimateCircleState animateCircleInit(int x0, int y0, int radius, float setPixelTime) {
  AnimateCircleState state = {0};
  state.x0 = x0;
  state.y0 = y0;
  state.radiusSquared = radius*radius;
  state.setPixelTime = setPixelTime;
  state.x = radius;
  state.currentOctant = 1;
  return state;
}

void animateCircle(AnimateCircleState *state, BackBuffer *bb, float dt) {
  if (state->x < state->y) {
    return;
  }

  state->t += dt;

  while (state->t >= state->setPixelTime) {
    state->t -= state->setPixelTime;

    switch (state->currentOctant) {
      case 1:
        setPixel(bb, state->x0 + state->x, state->y0 + state->y, COLOR_WHITE);
        break;
      case 2:
        setPixel(bb, state->x0 + state->y, state->y0 + state->x, COLOR_GREEN);
        break;
      case 3:
        setPixel(bb, state->x0 - state->y, state->y0 + state->x, COLOR_RED);
        break;
      case 4:
        setPixel(bb, state->x0 - state->x, state->y0 + state->y, COLOR_BLUE);
        break;
      case 5:
        setPixel(bb, state->x0 - state->x, state->y0 - state->y, COLOR_YELLOW);
        break;
      case 6:
        setPixel(bb, state->x0 - state->y, state->y0 - state->x, COLOR_MAGENTA);
        break;
      case 7:
        setPixel(bb, state->x0 + state->y, state->y0 - state->x, COLOR_CYAN);
        break;
      case 8:
        setPixel(bb, state->x0 + state->x, state->y0 - state->y, COLOR_PINK);
        break;
    }

    ++state->currentOctant;

    if (state->currentOctant > 8) {
      state->currentOctant = 1;
      state->y += 1;

      int ySquared = state->y*state->y;
      int oldXError = abs(state->x*state->x + ySquared - state->radiusSquared);
      int newXError = abs((state->x-1)*(state->x-1) + ySquared - state->radiusSquared);

      if (newXError < oldXError) {
        state->x -= 1;
      }
    }
  }
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

  BackBuffer bb = makeBackBuffer(GetDC(wnd), windowWidth, windowHeight, windowWidth/4, windowHeight/4);
  clearBackBuffer(&bb, COLOR_BLACK);

  bool running = true;

  AnimateCircleState animateCircleState = animateCircleInit(bb.width/2, bb.height/2, bb.height/4, 0.01f);

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

    //clearBackBuffer(&bb, COLOR_BLACK);
    animateCircle(&animateCircleState, &bb, dt);
    displayBackBuffer(&bb);
  }
}
