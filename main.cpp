#include <Windows.h>
#include <wtsapi32.h>
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
  if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) == 0)
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
}

void start() {
  startKana();
  startConv();
  startSpace(); // need call in this order
  startUS();    // actual call order: us -> space
}

void stop() {
  stopUS();
  stopSpace();
  stopConv();
  stopKana();
}

HWND msgwin;
LRESULT CALLBACK wtsMessage(HWND, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_WTSSESSION_CHANGE:
      switch (wParam) {
        case WTS_SESSION_LOCK:
          stop();
          break;
        case WTS_SESSION_UNLOCK:
          start();
          break;
      }
  }
  return 0;
}

HWND CreateMessageWindow(HINSTANCE hinst, WNDPROC windowProc) {
	TCHAR      szAppName[] = TEXT("messagewindow");
	HWND       hwnd;
	MSG        msg;
	WNDCLASSEX wc;

	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = 0;
	wc.lpfnWndProc   = windowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hinst;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szAppName;
	wc.hIconSm       = NULL;

	if (RegisterClassEx(&wc) == 0)
		return 0;

	return CreateWindowEx(0, szAppName, NULL, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hinst, NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdArgs, int nCmdShow) {
  //Win32Console console(true);
  setHighPriority();
  msgwin = CreateMessageWindow(hInstance, wtsMessage);
  if (msgwin)
    WTSRegisterSessionNotification(msgwin, NOTIFY_FOR_THIS_SESSION);

  start();

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

  stop();
  if (msgwin)
    WTSUnRegisterSessionNotification(msgwin);

  return 0;
}

