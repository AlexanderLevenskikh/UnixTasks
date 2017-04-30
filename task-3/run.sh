#!/bin/bash

printf "Generate file with 1000 random numbers from [1.10000] \n\n"
echo "fdsf " > test_file
bash files_generator.sh 1000 1 10000 test_file

g++ pr.cpp -o pr
printf "Execute program with generated test_file and output to result_file\n\n"
./pr test_file result_file 

printf "Add trash to file (non-decimal symbols) and output to result_file_1 \n\n"
printf "ff sf fs fsd fsd fds fsdf f sf f    fsdfs fsd fds f g  h r gdf  34  f 4  54g 55 g" >> test_file
./pr test_file result_file_1 

printf "Execute program with simulated 5 sec delay in child process (with --broke-sort key)\n"
./pr test_file result_file --broke-sort
