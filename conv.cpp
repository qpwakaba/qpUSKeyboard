#include <tchar.h>
#include <Windows.h>

namespace {
  constexpr ULONG_PTR DUMMY_SEND_FLAG = 0x1;
  constexpr ULONG_PTR US_INJECTED_FLAG = 0x2;
  constexpr ULONG_PTR SPACE_INJECTED_FLAG = 0x4;
  constexpr ULONG_PTR CONV_INJECTED_FLAG = 0x8;

  void sendKey(WORD vk, WORD sc, bool pressed, ULONG_PTR extraInfo = CONV_INJECTED_FLAG) {
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

  bool conv = false;
  bool convWhenTheKeyPressed = false;
  bool someKeyPressed = false;
  WORD pressedVk = 0;
  WORD pressedSc = 0;
  LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) goto CALL_NEXT_HOOK;

    LPKBDLLHOOKSTRUCT kb = (LPKBDLLHOOKSTRUCT) lParam;
    bool pressed = wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN;
    if (kb->dwExtraInfo & DUMMY_SEND_FLAG) return -1;
    if (kb->dwExtraInfo & CONV_INJECTED_FLAG) goto CALL_NEXT_HOOK;

    if (kb->scanCode == 123 /* NO_CONVERT */) {
      sendKey(VK_NONCONVERT, 123, pressed);
      return -1;
    } else if (kb->scanCode == 121 /* CONVERT */) {
      conv = pressed;
      if (pressed) {
        someKeyPressed = false;
      } else {
        sendKey(VK_CONVERT, 121, true);
        sendKey(VK_CONVERT, 121, false);
        if (someKeyPressed && pressedVk != 0) {
          sendKey(pressedVk, pressedSc, true);
          pressedVk = 0;
          pressedSc = 0;
        }
      }
      return -1;
    } else if (pressed && conv) {
      someKeyPressed = true;
      if (isIgnoreVK(kb->vkCode)) goto CALL_NEXT_HOOK;
      if (!getPressState(VK_CONTROL)) {
        convWhenTheKeyPressed = true;
        pressedVk = kb->vkCode;
        pressedSc = kb->scanCode;
      } else
        sendKey(kb->vkCode, kb->scanCode, true);
      return -1;
    } else if (conv && convWhenTheKeyPressed && pressedVk != 0) {
      sendKey(VK_LCONTROL, 0, true);
      sendKey(pressedVk, pressedSc, true);
      sendKey(VK_LCONTROL, 0, false);
      if (!pressed && kb->vkCode == pressedVk && kb->scanCode == pressedSc) {
        pressedVk = 0;
        pressedSc = 0;
        goto CALL_NEXT_HOOK;
      }
      if (kb->vkCode != pressedVk || kb->scanCode != pressedSc) {
        sendKey(VK_LCONTROL, 0, true);
        sendKey(pressedVk, pressedSc, true);
        sendKey(VK_LCONTROL, 0, false);
        if (pressed) {
          pressedVk = kb->vkCode;
          pressedSc = kb->scanCode;
        } else {
          pressedVk = 0;
          pressedSc = 0;
        }
      }
    }

    if (!pressed) goto CALL_NEXT_HOOK;

CALL_NEXT_HOOK:
    return CallNextHookEx(0, nCode, wParam, lParam);
  }


  HHOOK hook = NULL;
}

void startConv() {
    hook = SetWindowsHookEx(WH_KEYBOARD_LL, &LowLevelKeyboardProc, 0, 0);
}

void stopConv() {
    UnhookWindowsHookEx(hook);
}

