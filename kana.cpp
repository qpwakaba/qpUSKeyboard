#include <tchar.h>
#include <Windows.h>

namespace {
  constexpr ULONG_PTR DUMMY_SEND_FLAG = 0x1;
  constexpr ULONG_PTR US_INJECTED_FLAG = 0x2;
  constexpr ULONG_PTR SPACE_INJECTED_FLAG = 0x4;
  constexpr ULONG_PTR CONV_INJECTED_FLAG = 0x8;
  constexpr ULONG_PTR KANA_INJECTED_FLAG = 0x10;

  void sendKey(WORD vk, WORD sc, bool pressed, ULONG_PTR extraInfo = KANA_INJECTED_FLAG) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    input.ki.wScan = (sc != 0 ? sc : MapVirtualKey(vk, 0));
    input.ki.dwFlags = pressed ? 0 : KEYEVENTF_KEYUP;
    input.ki.dwExtraInfo = extraInfo;
    SendInput(1, &input, sizeof(INPUT));
  }

  bool getPressState(WORD vk) {
    return (GetKeyState(vk) & 0x8000) != 0;
  }

  bool isIgnoreVK(WORD vk) {
    switch (vk) {
      case VK_LCONTROL:
      case VK_RCONTROL:
      case VK_LSHIFT:
      case VK_RSHIFT:
      case VK_LMENU:
      case VK_RMENU:
      case VK_LWIN:
      case VK_RWIN:
      case VK_CONTROL:
      case VK_SHIFT:
      case VK_MENU:
        return true;
    }
    return false;
  }

  bool kana = false;
  bool someKeyPressed = false;
  LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) goto CALL_NEXT_HOOK;

    LPKBDLLHOOKSTRUCT kb = (LPKBDLLHOOKSTRUCT) lParam;
    bool pressed = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    if (kb->dwExtraInfo & DUMMY_SEND_FLAG) return -1;
    if (kb->dwExtraInfo & KANA_INJECTED_FLAG) goto CALL_NEXT_HOOK;

    if (kb->scanCode == 112 /* KANA */) {
        sendKey(VK_F24, 112, pressed);
        return 1;
    }


CALL_NEXT_HOOK:
    return CallNextHookEx(0, nCode, wParam, lParam);
  }


  HHOOK hook = NULL;
}

void startKana() {
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, 0, 0);
}

void stopKana() {
    UnhookWindowsHookEx(hook);
}

