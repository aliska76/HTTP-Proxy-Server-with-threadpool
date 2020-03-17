#!/bin/bash

FILE=$1

ID=$(echo $FILE | cut -d'_' -f 2)

mkdir $ID
tar -xvf $FILE -C $ID

cp threadpool.h filter.txt tester.sh tester_help.sh test0.sh test1.sh test2.sh test3.sh test4.sh test5.sh test6.sh ./$ID

cd $ID

./tester.sh $ID