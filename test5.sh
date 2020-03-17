#!/bin/bash

log_file=$1


echo "Description: Concurrent Requests" >> $log_file
echo "Expexted Result: keep alive" >> $log_file

seq 1 50 | xargs -P 8 -n 1 ./tester_help.sh

x=$(curl -s -o /dev/null -w "%{http_code}" http://www.google.com:80)

if [[ $x -eq 200 ]]; then
    echo "testpass"
else
    echo "testfail"
fi