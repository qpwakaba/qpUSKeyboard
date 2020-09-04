#include <Windows.h>
#include "Win32Console.hpp"
void startSpace();
void stopSpace();
void startUS();
void stopUS();
void startConv();
void stopConv();
void startKana();
void stopKana();

void setHighPriority() {
  SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

int WINAPI WinMain(HINSTANCE hIsntace, HINSTANCE hPrevInstance, LPSTR lpCmdArgs, int nCmdShow) {
  //Win32Console console(true);
  setHighPriority();

  startKana();
  startConv();
  startSpace(); // need call in this order
  startUS();    // actual call order: us -> space

  BOOL bRet;
  MSG msg;
  while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
    if (bRet == -1) {
      break;
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  stopUS();
  stopSpace();
  stopConv();
  stopKana();
  return 0;
}

