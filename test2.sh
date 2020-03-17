#!/bin/bash

log_file=$1


echo "Description: Forbidden URL" >> $log_file
echo "Expexted Result: 403 Forbidden" >> $log_file

x=$(curl -i --max-time 5 www.ynet.co.il:80 2>&1)
echo "Actual Result: $x" >> $log_file
echo "" >> $log_file

if [[ $x == *"403 Forbidden"* ]]; then
    echo "pass"
else
    echo "fail"
fi

