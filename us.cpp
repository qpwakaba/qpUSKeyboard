#include <Windows.h>
#include "Win32Console.hpp"
#include <cstdio>
#include "vktable.h"

namespace {
  constexpr ULONG_PTR DUMMY_SEND_FLAG = 0x1;
  constexpr ULONG_PTR US_INJECTED_FLAG = 0x2;
  constexpr ULONG_PTR SPACE_INJECTED_FLAG = 0x4;
  constexpr ULONG_PTR CONV_INJECTED_FLAG = 0x8;

  void sendKey(WORD vk, WORD sc, bool pressed, ULONG_PTR extraInfo = US_INJECTED_FLAG) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = sc;
    input.ki.dwFlags = pressed ? 0 : KEYEVENTF_KEYUP;
    input.ki.dwExtraInfo = extraInfo;
    if (!(extraInfo & DUMMY_SEND_FLAG))
      _tprintf(_T("%02X, %03X, %d, %s\n"), sc, extraInfo, pressed, vkname(vk));
    SendInput(1, &input, sizeof(INPUT));
  }

  bool getPressState(WORD vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
  }

  bool lastShiftWhenControlPressed = false;

  LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) goto CALL_NEXT_HOOK;

    LPKBDLLHOOKSTRUCT kb = (LPKBDLLHOOKSTRUCT) lParam;
    bool pressed = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    if (kb->dwExtraInfo & DUMMY_SEND_FLAG) return -1;
    if (kb->dwExtraInfo & US_INJECTED_FLAG) goto CALL_NEXT_HOOK;
    if (kb->dwExtraInfo & SPACE_INJECTED_FLAG) goto CALL_NEXT_HOOK;
    if (kb->dwExtraInfo & CONV_INJECTED_FLAG) goto CALL_NEXT_HOOK;
    switch (kb->vkCode) {
      case VK_LCONTROL:
        {
        bool shift = getPressState(VK_LSHIFT);
        bool control = getPressState(VK_LCONTROL);
        if (pressed && !control) lastShiftWhenControlPressed = shift;
        if (!getPressState(VK_LCONTROL))
          sendKey(VK_LCONTROL, kb->scanCode, false, DUMMY_SEND_FLAG);

        if (!pressed) sendKey(VK_LSHIFT, 44, lastShiftWhenControlPressed, DUMMY_SEND_FLAG);
        else if (shift) sendKey(VK_LSHIFT, 44, true, DUMMY_SEND_FLAG);

        sendKey(VK_CAPITAL, kb->scanCode, pressed);

        if (shift)
          sendKey(VK_LSHIFT, 44, true, DUMMY_SEND_FLAG);
        sendKey(VK_LCONTROL, kb->scanCode, false, DUMMY_SEND_FLAG);


        return -1;
        }
      case VK_LSHIFT:
      case VK_SHIFT:
        sendKey(kb->vkCode, kb->scanCode, pressed);
        if (pressed) sendKey(kb->vkCode, kb->scanCode, false, DUMMY_SEND_FLAG);
        return -1;
    }
    switch (kb->scanCode) {
      case 58: // caps lock
        if (pressed && getPressState(VK_SHIFT))
          sendKey(VK_SHIFT, 44, false, DUMMY_SEND_FLAG);
        if (getPressState(VK_LCONTROL) != pressed)
          sendKey(VK_LCONTROL, 58, pressed);
        sendKey(VK_LCONTROL, 58, false, DUMMY_SEND_FLAG);
        return -1;
      case 125: //jis: yen/pipe
        sendKey(VK_BACK, kb->scanCode, pressed);
        return -1;
      case 115: // jis:backslash/underscore
        sendKey(VK_RSHIFT, kb->scanCode, pressed);
        return -1;
      case 43: // us: \|
        sendKey(VK_BACK, kb->scanCode, pressed);
        return -1;
      case 14: // backspace
        sendKey(VK_OEM_5, kb->scanCode, pressed);
        return -1;
    }
CALL_NEXT_HOOK:
    return CallNextHookEx(0, nCode, wParam, lParam);
  }

  TCHAR* AllocAndGetExecutablePath(size_t* outBufLen = nullptr) noexcept {
    size_t buflen = _MAX_PATH;
    TCHAR stackbuf[_MAX_PATH] = {};
    TCHAR* buf = stackbuf;
    auto len = GetModuleFileName(NULL, buf, buflen);
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      buflen = 32769;
      buf = new TCHAR[buflen];
      len = GetModuleFileName(NULL, buf, buflen);
      if (GetLastError() != ERROR_SUCCESS) {
        delete[] buf;
        return NULL;
      }
    }

    TCHAR* retbuf = new TCHAR[len + 1];
    _tcscpy_s(retbuf, len + 1, buf);
    if (buf != stackbuf) delete[] buf;
    if (outBufLen != nullptr) {
      *outBufLen = len + 1;
    }
    return retbuf;
    return nullptr;
  }

  bool exitPrevInstance() {
    return false;
    size_t buflen;
    const TCHAR* path = AllocAndGetExecutablePath(&buflen);


  }

  int WINAPI _WinMain(HINSTANCE hIsntace, HINSTANCE hPrevInstance, LPSTR lpCmdArgs, int nCmdShow) {
    //Win32Console console;

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, 0, 0);

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
    UnhookWindowsHookEx(hook);
    return 0;
  }

  HHOOK hook;
}

void startUS() {
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, 0, 0);
}

void stopUS() {
    UnhookWindowsHookEx(hook);
}
