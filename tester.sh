#!/bin/bash

log_file="$1.log"
test_record=0
grade_record=$1

PORT=8080

echo "------------ TEST 0 (Compilation) ------------" >> $log_file

x=$(./test0.sh $log_file)

if [[ $x == *"clean"* ]]; then
    echo "Test Result: Clean Compilation" >> $log_file
    test_record=1
elif [[ $x == *"warn"* ]]; then
    echo "Test Result: Compilation Warnings" >> $log_file
    test_record=0.5
else
    echo "Test Result: Compilation Errors" >> $log_file
    test_record=0
    grade_record="$grade_record,$test_record"
    echo "$grade_record" >> ../grades.csv
    cp $log_file ../logs
    exit 1
fi

grade_record="$grade_record,$test_record"

echo "---------- END TEST 0 (Compilation) ----------" >> $log_file

###############################################################

echo "------------ TEST 1 ------------" >> $log_file
export http_proxy="http://localhost:$PORT/"
./server $PORT 2 5 "filter.txt" &
sleep 1

x=$(timeout 10 ./test1.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"pass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 1 ----------" >> $log_file

###############################################################

echo "------------ TEST 2 ------------" >> $log_file
PORT=$((PORT+1))
export http_proxy="http://localhost:$PORT/"
./server $PORT 2 5 "filter.txt" &
sleep 1

x=$(timeout 10 ./test2.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"pass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 2 ----------" >> $log_file

###############################################################

echo "------------ TEST 3 ------------" >> $log_file
PORT=$((PORT+1))
export http_proxy="http://localhost:$PORT/"
./server $PORT 2 5 "filter.txt" &
sleep 1

x=$(timeout 10 ./test3.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"pass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 3 ----------" >> $log_file

###############################################################

echo "------------ TEST 4 ------------" >> $log_file
PORT=$((PORT+1))
export http_proxy="http://localhost:$PORT/"
./server $PORT 1 2 "filter.txt" &
sleep 1

x=$(timeout 20 ./test4.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"pass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 4 ----------" >> $log_file

###############################################################

echo "------------ TEST 5 ------------" >> $log_file
PORT=$((PORT+1))
export http_proxy="http://localhost:$PORT/"
./server $PORT 10 100 "filter.txt" &
sleep 1

x=$(timeout 30 ./test5.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"testpass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 5 ----------" >> $log_file

###############################################################

echo "------------ TEST 6 ------------" >> $log_file
PORT=$((PORT+1))
export http_proxy="http://localhost:$PORT/"
./server $PORT 1 100 "filter.txt" &
sleep 1

x=$(timeout 40 ./test6.sh $log_file || echo "testtimeout")
echo $x

if [[ $x == *"testpass"* ]]; then
    echo "Test Result: PASS" >> $log_file
    test_record=1
elif [[ $x == *"testtimeout"* ]]; then
    echo "Test Result: FAIL (Timeout)" >> $log_file
    test_record=0
else
    echo "Test Result: FAIL" >> $log_file
    test_record=0
fi

grade_record="$grade_record,$test_record"

lsof -t -i tcp:$PORT | xargs kill -9
echo "---------- END TEST 6 ----------" >> $log_file


echo "$grade_record" >> ../grades.csv
cp $log_file ../logs