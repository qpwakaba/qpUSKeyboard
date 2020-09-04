#pragma once
#include <Windows.h>
#include <tchar.h>
#include <cstdio>


class Win32Console final {
private:
  bool needFreeConsole;
public:
  inline Win32Console(bool allocIfNoConsole = false) {
    BOOL bRet = AttachConsole(ATTACH_PARENT_PROCESS);
    if (!bRet && !allocIfNoConsole) return;

    //MessageBox(NULL, bRet ? _T("TRUE") : _T("FALSE"), _T("test - AttachConsole"), MB_OK);
    if (!bRet) {
      // エクスプローラから起動した場合は新規にコンソールを割り当てる
      if (!AllocConsole()) return;
    }
    freopen("CONIN$", "r", stdin);  // "CONIN$", "CONOUT$", "CON" の違いがよく分かっていない
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    needFreeConsole = true;
  }
  ~Win32Console() {
    if (needFreeConsole) {
      FreeConsole();
      needFreeConsole = false;
    }
  }
};
