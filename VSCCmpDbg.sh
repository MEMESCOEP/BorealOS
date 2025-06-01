#!/bin/bash
# Compile only if it's allowed
if [ "$2" != "--OnlyDBG" ]; then
    make clean && make
    
    if [ $? -eq 0 ]; then
        echo "Build succeeded"
    else
        echo "Build failed"
        exit 1
    fi
fi

# Start the VM if debugging is allowed
if [ "$1" != "--NoDBG" ]; then
    case "$1" in
        --Bochs)
            bochs -q -f BochsConfig-Linux.cfg
            ;;
        --QEMU)
            ./QEMU.sh
            ;;
        *)
            echo "Invalid virtual machine option \"$1\"."
            exit 1
            ;;
    esac
fi