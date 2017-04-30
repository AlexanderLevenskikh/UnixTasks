#!/bin/bash

count=$1
left_border=$2
right_border=$3
filename=$4

if [[ ($1 -eq "-h") || ($1 -eq "--help") ]]; then
    echo "Usage: hugeFileGenerator numbers_count left_border right_border filename"
fi

for i in `seq 1 $count`; do 
    shuf -i $left_border-$right_border -n 1 >> $filename
done;

