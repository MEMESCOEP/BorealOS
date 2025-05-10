#!/bin/bash
if [ "$1" == "--OnlyDBG" ]; then
    ./QEMU.sh
else
    make clean && make && ./MakeISO.sh

    if [ "$1" != "--NoDBG" -a $? -eq 0 ]; then
        ./QEMU.sh
    fi
fi