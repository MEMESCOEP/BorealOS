#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <output_image_path> <size>"
    exit 1
fi

OUTPUT_IMAGE_PATH=$1
SIZE=$2

qemu-img create -f qcow2 "$OUTPUT_IMAGE_PATH" "$SIZE"