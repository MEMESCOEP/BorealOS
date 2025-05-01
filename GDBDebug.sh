#!/bin/sh
gdb -ex "target remote :1234" -ex "file bin/BorealOS"
