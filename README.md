# BorealOS
A barebones x86 terminal-based OS


### CLion specific stuff:
If you want to debug using CLion's integrated GDB, you can do so by running the remote debug target "Debug". But, for this to work there must be an external tool called 'Run Debug' which runs `/bin/bash` as program, with `$ProjectFileDir$/debug.sh &` as arguments and `$ProjectFileDir$` as working directory. This is so the QEMU emulator runs on a seperate thread, so it does not block the CLion "before launch" sequence.

