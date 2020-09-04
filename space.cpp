#include <tchar.h>
#include <Windows.h>
#include "VirtualKeyCodes.h"
#include "Finally.hpp"

namespace {
  constexpr ULONG_PTR DUMMY_SEND_FLAG = 0x1;
  constexpr ULONG_PTR US_INJECTED_FLAG = 0x2;
  constexpr ULONG_PTR SPACE_INJECTED_FLAG = 0x4;
  constexpr ULONG_PTR CONV_INJECTED_FLAG = 0x8;

  HHOOK hook;
  int mapping[0x100] = {};

  void InitializeMapping();

  LRESULT CALLBACK LowLevelKeyboardProc(
      int nCode,     // フックコード
      WPARAM wParam, // メッセージ識別子
      LPARAM lParam  // メッセージデータ
      );
  bool getKeyState(int vkCode) {
    return GetAsyncKeyState(vkCode) & 0x8000;
  }
  LPTSTR AllocateAndGetErrorMessage(int err) {
    LPTSTR errorText = NULL;

    FormatMessage(
        // use system message tables to retrieve error text
        FORMAT_MESSAGE_FROM_SYSTEM
        // allocate buffer on local heap for error text
        |FORMAT_MESSAGE_ALLOCATE_BUFFER
        // Important! will fail otherwise, since we're not 
        // (and CANNOT) pass insertion parameters
        |FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,    // unused with FORMAT_MESSAGE_FROM_SYSTEM
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &errorText,  // output 
        0, // minimum size for output buffer
        NULL);   // arguments - see note 
    return errorText;
  }
  void OutputDebugErrorString(int err) {
    LPTSTR errorText = AllocateAndGetErrorMessage(err);
    if (NULL != errorText)
    {
      // ... do something with the string `errorText` - log it, display it to the user, etc.
      OutputDebugString(errorText);
      // release memory allocated by FormatMessage()
      LocalFree(errorText);
      errorText = NULL;
    }
  }
  void SendKey(int vkCode, int scanCode, bool toPress) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.wScan = scanCode;
    input.ki.dwFlags = toPress ? 0 : KEYEVENTF_KEYUP;
    input.ki.dwExtraInfo = SPACE_INJECTED_FLAG;
    switch (vkCode) {
      case VK_LEFT:
      case VK_RIGHT:
      case VK_UP:
      case VK_DOWN:
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
  }

  void SendKey(int vkCode, bool toPress) {
    int scanCode = MapVirtualKey(vkCode, 0);

    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vkCode;
    input.ki.wScan = scanCode;
    input.ki.dwFlags = toPress ? 0 : KEYEVENTF_KEYUP;
    input.ki.dwExtraInfo = SPACE_INJECTED_FLAG;
    switch (vkCode) {
      case VK_LEFT:
      case VK_RIGHT:
      case VK_UP:
      case VK_DOWN:
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    SendInput(1, &input, sizeof(INPUT));
  }

  unsigned int GetKeyboradDelay() {
    int delay;
    if (SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &delay, 0)) {
      return (delay + 1) * 250;
    }
    return 250;
  }

  unsigned int GetSpaceDelay() {
    return GetDoubleClickTime() / 4;
  }

  constexpr size_t tstrlen(const TCHAR* str) {
    size_t len = 0;
    for (;str[len];++len);
    return len;
  }
  const TCHAR* Minecraft = _T("Minecraft");
  const size_t Minecraft_len = tstrlen(_T("Minecraft"));
  const TCHAR* Minecraft_1 = _T("Minecraft 1");
  const size_t Minecraft_1_len = tstrlen(_T("Minecraft 1"));

  bool startsWith(const TCHAR* str, const TCHAR* target, size_t length) {
    for (size_t i = 0; i < length; ++i) {
      if (target[i] == 0) return true;
      if (str[i] == 0) return false;
      if (str[i] != target[i]) return false;
    }
    return true;
  }

  bool equals(const TCHAR* x, const TCHAR* y) {
    if (*x != *y) return false;
    else if (!*x) return true;
    else return equals(x + 1, y + 1);
  }

  bool isIgnoreApplication() {
    HWND active = GetForegroundWindow();
    TCHAR text[Minecraft_1_len + 1];
    size_t read = GetWindowText(active, text, Minecraft_1_len + 1);
    if (equals(text, Minecraft)) return true;
    return startsWith(text, Minecraft_1, Minecraft_1_len);
  }

  bool space = false;
  ULONGLONG spaceLast = 0;
  bool spaceLastFlag = false;
  bool firstSpaceCommand = true;
  int firstInputVK = 0;
  int firstInputSC = 0;
  int repeating = 0;
  int repeatingReset = 0;
  bool repeatingResetFlag = false;
#define IsPressing(__kb) (!((__kb)->flags >> 7))

  void SpaceCommand(int vkCode, int scanCode, bool pressing);
  LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode < 0) return CallNextHookEx(hook, nCode, wParam, lParam);
    LPKBDLLHOOKSTRUCT kb = (LPKBDLLHOOKSTRUCT) lParam;
    if (kb->dwExtraInfo & DUMMY_SEND_FLAG) return -1;
    if (kb->dwExtraInfo & SPACE_INJECTED_FLAG) return CallNextHookEx(hook, nCode, wParam, lParam);
    if (kb->dwExtraInfo & CONV_INJECTED_FLAG) return CallNextHookEx(hook, nCode, wParam, lParam);
    if (isIgnoreApplication()) return CallNextHookEx(hook, nCode, wParam, lParam);
    if (kb->vkCode == repeating && repeatingResetFlag && GetTickCount64() - repeatingReset < GetKeyboradDelay())
      return -1;
    repeating = 0;
    repeatingResetFlag = false;

    if (kb->vkCode == VK_SPACE) {
      bool callNextHook = false;
      // ignore key repeating
      if (space && IsPressing(kb)) return -1;
      space = IsPressing(kb);
      if (space) {
        spaceLast = GetTickCount();
        spaceLastFlag = true;
      }
      else {
        callNextHook = true;
        if (firstInputVK > 0) {
          repeating = 0;
          if (GetTickCount() - spaceLast < 1.2 * GetSpaceDelay()) {
            //スペース→文字→スペース離し
            SendKey(VK_SPACE, true);
            SendKey(firstInputVK, firstInputSC, true);
            SendKey(VK_SPACE, false);
            firstInputVK = 0;
            return -1;
          }
          else {
            SpaceCommand(firstInputVK, firstInputSC, true);
            firstInputVK = 0;
            return -1;
          }
        }
        else if (firstInputVK != 0) {
          firstInputVK = 0;
          if (repeating > 0) {
            SendKey(repeating, false);
            repeatingReset = GetTickCount();
            repeatingResetFlag = true;
          }
          return -1;
        }
        else if (spaceLastFlag && GetTickCount() - spaceLast < GetDoubleClickTime() / 3) {
          SendKey(VK_SPACE, true);
          SendKey(VK_SPACE, false);
          //space == false
        }
      }
      if (false && callNextHook) return CallNextHookEx(hook, nCode, wParam, lParam);
      else return -1;
    }
    else if (space) {
      // スペースを押しながら文字を押す
      //   文字を離したらスペースコマンド
      //   1回目の文字入力でスペースを離したら，スペース→文字→スペース離す の順に入力 DONE
      //   2回目以降の文字入力でスペースを離したらスペースコマンド
      if (IsPressing(kb)) {
        repeating = kb->vkCode;
        if (spaceLastFlag)
          spaceLastFlag = false;
      }
      else {
        repeating = 0;
      }
      switch (kb->vkCode) {
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
          return CallNextHookEx(hook, nCode, wParam, lParam);
      }
      if (firstInputVK == 0 && IsPressing(kb) && GetTickCount() - spaceLast < GetSpaceDelay()) {
        firstInputVK = kb->vkCode;
        firstInputSC = kb->scanCode;
        return -1;
      }
      else if (firstInputVK > 0 && IsPressing(kb)) {
        //スペースコマンド
        SpaceCommand(firstInputVK, firstInputSC, true);
        firstInputVK = -1;
        SpaceCommand(kb->vkCode, kb->scanCode, true);
        return -1;
      }
      else if (kb->vkCode == firstInputVK) {
        //スペースコマンド
        SpaceCommand(firstInputVK, firstInputSC, true);
        SpaceCommand(firstInputVK, firstInputSC, IsPressing(kb));
        firstInputVK = -1;
        return -1;
      }
      else {
        SpaceCommand(kb->vkCode, kb->scanCode, IsPressing(kb));
        return -1;
      }
    }
    repeatingResetFlag = false;
    return CallNextHookEx(hook, nCode, wParam, lParam);
  }

void SpaceCommand(int vkCode, int scanCode, bool pressing) {
  bool shift = getKeyState(VK_SHIFT);

  if (!shift && pressing) {
    SendKey(VK_SHIFT, true);
    SendKey(vkCode, scanCode, true);
    SendKey(VK_SHIFT, false);
  }
  else {
    SendKey(vkCode, scanCode, pressing);
  }
  return;

  /*
     if (vkCode == VK_Z) {
     bool ctrl = getKeyState(VK_LCONTROL);
     if (!ctrl)
     SendKey(VK_LCONTROL, true);
     SendKey(VK_Z, pressing);
     if (!ctrl)
     SendKey(VK_LCONTROL, false); 
     }
     else {
     int converted = mapping[vkCode];
     if (converted) {
     SendKey(converted, pressing);
     }
     }
   */
}
void InitializeMapping() {
  mapping[VK_E] = VK_UP;
  mapping[VK_S] = VK_LEFT;
  mapping[VK_D] = VK_DOWN;
  mapping[VK_F] = VK_RIGHT;
  mapping[VK_W] = VK_HOME;
  mapping[VK_R] = VK_END;
  mapping[VK_A] = VK_HOME;
  mapping[VK_G] = VK_END;

  mapping[VK_K] = VK_RETURN;
  mapping[VK_J] = VK_BACK;
  mapping[VK_L] = VK_DELETE;
}
}
void startSpace() {
  InitializeMapping();
  hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, 0, 0);
  if (hook == NULL) {
    OutputDebugErrorString(GetLastError());
  }
}
void stopSpace() {
  UnhookWindowsHookEx(hook);
}

