#!/bin/bash

processBar()
{
    now=$1
    all=$2
    percent=`awk BEGIN'{printf "%f", ('$now'/'$all')}'`
    len=`awk BEGIN'{printf "%d", (100*'$percent')}'`
    bar='>'
    for((i=0;i<len-1;i++))
    do
        bar="="$bar
    done
    printf "[%-100s][%03d/%03d]\r" $bar $len 100
}

whole=100
process=0
while [ $process -lt $whole ] 
do
    let process++
    processBar $process $whole
    sleep 0.1
done
printf "\n"
