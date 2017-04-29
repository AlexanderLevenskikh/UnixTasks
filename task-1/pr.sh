#!/bin/bash 

game_pipe=/tmp/game_pipe
state=0 # 1 - filled
symbol=0 # 1 - x, 0 - o

isFirst=true
isPlayer=true

function initGame(){
    tput civis
    tput clear
    printHeader
    printContent
    printFooterHelp
}

function printHeader() {
    tput cup 1 3
    tput bold
    echo tic tac game
    tput sgr0
}

function printCurrentSymbol() {
    # $1 == i, $2 == j
    local pow2;
    local mask;
    local isFilled;
    local symbolBit;
    
    let "pow2 = $1 * 3 + $2"
    let "mask = 2 ** pow2"
    let "isFilled = state & mask"
    if [[ $isFilled != 0 ]]; then 
        let "symbolBit = symbol & mask"
        if [[ $symbolBit != 0 ]]; then
            echo x
        else 
            echo o
        fi
    else
        echo .
    fi
}

function printContent() {
    local i=0
    local j=0
    local row=3
    local column=3
    
    #print the game board
    while (( i <= 2 )); do
       let "row = i + 3"
       let "column = 2 * (j + 1) + 4"
       tput cup $row $column
       
        #calculate the current symbol from $state and current position
       printCurrentSymbol $i $j
       
       ((j++))
       if [[ $j = 3 ]]
       then 
            j=0
            ((i++))
       fi
    done
}

function printFooterHelp() {
    local mySymbol=o
    
    if $isPlayer; then 
        if $isFirst; then 
            mySymbol=x
        fi
        
        tput cup 7 3
        echo Your symbol is "$mySymbol"

        tput cup 9 3
        echo Press 1-9 to make a turn
        tput cup 10 3
        echo 1 2 3
        tput cup 11 3
        echo 4 5 6
        tput cup 12 3
        echo 7 8 9
    else 
        tput cup 7 3
        echo You are the game observer
    fi
    
    
    
}

initGame;

while :; do
    :;
done;