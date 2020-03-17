#!/bin/bash

log_file=$1


compiled=$(gcc -std=gnu99 -Wall *.c threadpool.h -o server -lpthread 2>&1)

echo "$compiled" >> $log_file

if [ -z "$compiled" ]; then
    echo "clean"
    exit 0
fi

if [[ $compiled == *"error:"* ]]; then
    echo "error"
    exit 1
fi

if [[ $compiled == *"warning:"* ]]; then
    echo "warn"
    exit 1
fi