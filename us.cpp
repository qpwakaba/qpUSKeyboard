#include <Windows.h>
#include "Win32Console.hpp"
#include <cstdio>
#include "vktable.h"
#include "Finally.hpp"

namespace {
  constexpr ULONG_PTR DUMMY_SEND_FLAG = 0x1;
  constexpr ULONG_PTR US_INJECTED_FLAG = 0x2;
  constexpr ULONG_PTR SPACE_INJECTED_FLAG = 0x4;
  constexpr ULONG_PTR CONV_INJECTED_FLAG = 0x8;

  WORD getScancodeFromVk(WORD vk) {
    return (WORD)MapVirtualKeyW(vk, MAPVK_VK_TO_VSC_EX);
  }

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

  void sendKey(WORD vk, bool pressed, ULONG_PTR extraInfo = US_INJECTED_FLAG) {
    sendKey(vk, getScancodeFromVk(vk), pressed, extraInfo);
  }

  bool getPressState(WORD vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
  }

  bool lastShiftWhenControlPressed = false;

  bool _lshift = getPressState(VK_LSHIFT);
  bool _lctrl = getPressState(VK_LCONTROL);
  bool _lalt = getPressState(VK_LMENU);

  bool usEnabled = true;

  LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) goto CALL_NEXT_HOOK;

    LPKBDLLHOOKSTRUCT kb = (LPKBDLLHOOKSTRUCT) lParam;
    bool pressed = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    if (kb->dwExtraInfo & DUMMY_SEND_FLAG) return -1;
    if (kb->dwExtraInfo & US_INJECTED_FLAG) goto CALL_NEXT_HOOK;
    if (kb->dwExtraInfo & SPACE_INJECTED_FLAG) goto CALL_NEXT_HOOK;
    if (kb->dwExtraInfo & CONV_INJECTED_FLAG) goto CALL_NEXT_HOOK;

    if (kb->vkCode == VK_PAUSE && pressed && getPressState(VK_LSHIFT))
          usEnabled = !usEnabled;
    if (!usEnabled) goto CALL_NEXT_HOOK;
    {
    sendKey(VK_LSHIFT, 44, _lshift, DUMMY_SEND_FLAG);
    sendKey(VK_LCONTROL, 58, _lctrl, DUMMY_SEND_FLAG);
    sendKey(VK_LMENU, 58, _lalt, DUMMY_SEND_FLAG);
    auto for_msime = finally([]() {
      _lshift = getPressState(VK_LSHIFT);
      _lctrl = getPressState(VK_LCONTROL);
      _lalt = getPressState(VK_LMENU);
      sendKey(VK_LSHIFT, 44, false, DUMMY_SEND_FLAG);
      sendKey(VK_LCONTROL, 58, false, DUMMY_SEND_FLAG);
      sendKey(VK_LMENU, 58, false, DUMMY_SEND_FLAG);
    });
    switch (kb->vkCode) {
      case VK_LCONTROL:
        {
        bool shift = getPressState(VK_LSHIFT);
        bool control = getPressState(VK_LCONTROL);
        if (pressed && !control) lastShiftWhenControlPressed = shift;
        sendKey(VK_CAPITAL, pressed);
        return -1;
        }
      case VK_LSHIFT:
      case VK_SHIFT:
        sendKey(kb->vkCode, pressed);
        return -1;
      case VK_PAUSE:
        {
        
        break;
        }
    }
    switch (kb->scanCode) {
      case 58: // caps lock
        if (getPressState(VK_LCONTROL) != pressed)
          sendKey(VK_LCONTROL, pressed);
        return -1;
      case 125: //jis: yen/pipe
        sendKey(VK_BACK, pressed);
        return -1;
      case 115: // jis:backslash/underscore
        sendKey(VK_RSHIFT, pressed);
        return -1;
      case 43: // us: \|
        sendKey(VK_BACK, pressed);
        return -1;
      case 14: // backspace
        sendKey(VK_OEM_5, pressed);
        return -1;
    }
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
