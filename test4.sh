#!/bin/bash

log_file=$1


echo "Description: Max Requests" >> $log_file
echo "Expexted Result: 2/3 Succeeded: [200, 200, 000]" >> $log_file

x1=$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com:80)
echo "Request 1: $x1" >> $log_file

x2=$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com:80)
echo "Request 2: $x2" >> $log_file

x3=$(curl --max-time 5 -s -o /dev/null -w "%{http_code}" http://www.google.com:80)
echo "Request 3: $x3" >> $log_file
echo "" >> $log_file

echo "Actual Result: [$x1, $x2, $x3]" >> $log_file
echo "" >> $log_file

if [[ ( $x1 -eq 200 ) && ( $x2 -eq 200 ) && ( $x3 -eq 000 ) ]]; then
    echo "pass"
else
    echo "fail"
fi