VCVERSALL = "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
CMD = cmd.exe /Q
CC = $(CMD) /c @ECHO OFF \& $(VCVERSALL) x86\>NUL \& cl.exe
CC_X64 = $(CMD) /c @ECHO OFF \& $(VCVERSALL) amd64\>NUL \& cl.exe
CC_ARM = $(CMD) /c @ECHO OFF \& $(VCVERSALL) amd64_arm\>NUL \& cl.exe
CC_ARM64 = $(CMD) /c @ECHO OFF \& $(VCVERSALL) amd64_arm64\>NUL \& cl.exe
PROGRAM = qpUSKeyboard.exe
CFLAGS = /DUNICODE /D_UNICODE /MD /O2
LIBS = User32.lib Wtsapi32.lib

all: $(PROGRAM)

$(PROGRAM): conv.obj kana.obj main.obj space.obj us.obj
	$(CC_X64) $(CFLAGS) /Fe$@ $^ $(LIBS)

%.obj: %.cpp
	$(CC_X64) $(CFLAGS) /c /Fo$@ $^

$(wildcard *.cpp): $(wildcard *.hpp)

clean:
	rm *.exe *.obj 2>/dev/null >/dev/null || true
