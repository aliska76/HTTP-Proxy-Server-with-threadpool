#!/bin/bash

log_file=$1


echo "Description: Unsupported Method" >> $log_file
echo "Expexted Result: 501 Not supported" >> $log_file

x=$(curl -i --max-time 5 -X POST www.bbb.co.il:80 2>&1)
echo "Actual Result: $x" >> $log_file
echo "" >> $log_file

if [[ $x == *"501 Not supported"* ]]; then
    echo "pass"
else
    echo "fail"
fi
