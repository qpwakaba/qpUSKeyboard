all: qpUSKeyboard.exe
	cmd.exe /c dev \& make

debug: qpUSKeyboard.exe
	cmd.exe /c dev \& make-debug

