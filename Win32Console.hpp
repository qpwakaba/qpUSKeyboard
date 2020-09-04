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
      // �G�N�X�v���[������N�������ꍇ�͐V�K�ɃR���\�[�������蓖�Ă�
      if (!AllocConsole()) return;
    }
    freopen("CONIN$", "r", stdin);  // "CONIN$", "CONOUT$", "CON" �̈Ⴂ���悭�������Ă��Ȃ�
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
