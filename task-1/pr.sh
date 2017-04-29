#!/bin/bash 

trap exitHandler EXIT

game_pipe=/tmp/game_pipe
helper_pipe=/tmp/helper_pipe

state=0 # 1 - filled
symbol=0 # 1 - x, 0 - o

isFirst=true
isPlayer=true
isMyTurn=true

winConfigurationsMasks=( 7 56 448 73 146 292 273 84 )

function exitHandler() {
    tput sgr0
    tput cnorm
    stty echo
    rm -f $game_pipe 	
    rm -f $helper_pipe
    tput clear
    exit
}

function initGame() {
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
        
        tput cup 11 3
        echo Your symbol is "$mySymbol"

        tput cup 13 3
        echo Press 1-9 to make a turn
        tput cup 14 3
        echo 1 2 3
        tput cup 15 3
        echo 4 5 6
        tput cup 16 3
        echo 7 8 9
    else 
        tput cup 7 3
        echo You are the game observer
    fi
}

function makeTurn() {
    # $1 == $turn
    # $2 == my (true) or enemy (false) turn
    local mask
    local turn=$1
    let "mask = 2 ** (turn - 1)"
    let "state = state | mask"
    

    if [[ (($isFirst = true) && ($2 = true)) || (($isFirst = false) && ($2 = false)) ]]; then
        let "symbol = symbol | mask"
    fi
}

function isAnyPlayerWin() {
    local result=false
    local winner;
    
    for i in "${winConfigurationsMasks[@]}"
    do
       let "resultStateAnd = state & i"
       let "resultSymbolAnd = symbol & i"
       
       if [[ $resultStateAnd = $i ]]; then
            if [[ $resultSymbolAnd = 0 ]]; then
                result=true;
                winner=Second
            elif [[ $resultSymbolAnd = $i ]]; then 
                result=true;
                winner=First
            fi
       fi
    done
    
    if $result; then    
        tput cup 8 3
        tput ed
        echo "$winner player is win!"
        tput cup 9 3
        echo "Press any key to exit"
        read -n 1
        exitHandler       
    fi
}

function isDraw() {
    if [[ $state = 511 ]]; then
        tput cup 8 3
        tput ed
        echo "Draw!"
        tput cup 9 3
        echo "Press any key to exit"
        read -n 1
        exitHandler  
    fi
}


if [[ -p $game_pipe ]]; then
        isFirst=false
        if [[ -p $helper_pipe ]]; then
            isPlayer=false
        else 
            mkfifo $helper_pipe
            isMyTurn=false
        fi
else
	mkfifo $game_pipe
fi

initGame;


if $isPlayer; then 
    while :; do
        if $isMyTurn; then 
            tput cup 8 3
            tput el
            echo Your turn
            
            turn=-
            while  [[ !("$turn" =~ [1-9]) ]]; do 
                stty -echo
                read -n 1 turn
                stty echo
                tput cup 9 3
                tput el
                echo Only 1-9!
            done;
            
            let "isCellEmptyMask = 2 ** (turn - 1)"
            let "isCellEmptyResult = state & isCellEmptyMask"
            
            if [[ $isCellEmptyResult = 0 ]]; then
                stty echo
                tput cup 9 3
                tput el
                echo Your turn is $turn
                echo "$turn" > "$game_pipe"
                makeTurn $turn $isMyTurn
                
                let "helperPipeValue = state * 512 + symbol"
                echo "$helperPipeValue" > "$helper_pipe" &
                
                printContent
                isAnyPlayerWin
                isDraw
                
                isMyTurn=false
            else 
                tput cup 9 3
                tput el
                echo Cell $turn is not empty
            fi           
            
        else 
            tput cup 8 3
            tput el
            echo Wait the enemy move
            stty -echo
            read enemyTurn < "$game_pipe" 
            makeTurn $enemyTurn $isMyTurn
            
            printContent
            isAnyPlayerWin
            isDraw
            
            isMyTurn=true
        fi
    done;
else 
    read observerData < "$helper_pipe"
    echo $observerData
fi