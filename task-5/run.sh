#!/bin/bash

tput clear

printf "Execute the program\n\n";
./pr-levenskikh;

pid=$(ps -ejH | grep pr-levenskikh | awk 'NR==1 {print; exit}' | awk '{print $1;}'); 
printf "Process tree:\n"
pstree $pid -p

printf "Send the sighup to %d\n\n", $pid
kill -SIGHUP $pid;

printf "Wait 6 sec\n\n"

count=0
printf "Process tree:\n"
for count in `seq 1 6`;
do
        tput sc
	tput ed
	pstree $pid -p	
	if [[ $count != 6 ]]; then
		tput rc
	fi
	sleep 1
done 

printf "\n\n";

while true; do
    read -p "Do you wish to kill the process?" yn
    case $yn in
        [Yy]* ) kill $pid; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done
