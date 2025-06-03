#!/usr/bin/bash

# Check if ./Logs exists, if not create it
if [ ! -d "./Logs" ]; then
    mkdir ./Logs
fi

# Check if a HDD.img file exists, if not create it (qemu-img create -f raw HDD.img 2G)
if [ ! -f "./HDD.img" ]; then
    qemu-img create -f raw HDD.img 2G
fi

make clean
make
./QEMU.sh