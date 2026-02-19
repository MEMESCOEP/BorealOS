#!/bin/bash

# Arguments: 1 is the path to the directory to be archived, 2 is the output file path
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_directory> <output_file>"
    exit 1
fi

INPUT_DIRECTORY=$1
OUTPUT_FILE=$2

find "$INPUT_DIRECTORY" -print0 | cpio --null -ov --format=newc > "$OUTPUT_FILE"