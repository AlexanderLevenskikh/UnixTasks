#!/bin/bash

if [[ ! -f file_sparse ]]; then
    truncate -s 4096 file_sparse
fi

gcc pr.c -o pr
gzip -c file_sparse | gzip -cd | ./pr file_unzipped_sparse 

ls -ls --block-size 1 file_*
