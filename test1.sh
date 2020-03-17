#!/bin/bash

log_file=$1


echo "Description: Non existing URL" >> $log_file
echo "Expexted Result: 404 Not Found" >> $log_file

x=$(curl -i --max-time 5 www.sdf4fdf.com:80 2>&1)
echo "Actual Result: $x" >> $log_file
echo "" >> $log_file

if [[ $x == *"404 Not Found"* ]]; then
    echo "pass"
else
    echo "fail"
fi

