#!/bin/bash

trap "clean_all" SIGTERM SIGKILL SIGINT
FIFO=/tmp/tttgame

if [[ ! -p "$FIFO" ]]; then
    rm $FIFO 2>/dev/null
    mknod $FIFO p
    YOU='X'
    OPP='0'
    CUR_MOVE=true
else
    YOU='0'
    OPP='X'
    CUR_MOVE=false
fi

declare -A game_field
EMPTY_CELL='.'

for ((i=0; i<3; i++)) do
    for ((j=0; j<3; j++)) do
        game_field[$i,$j]="$EMPTY_CELL"
    done
done

function clean_all() {
    rm $FIFO 2>/dev/null
    exit
}

function draw_header() {
    echo "You: $YOU"
    echo "Input: row column"
    echo "You can enter from 1 to 3"
    echo
    echo "$INFO"
    echo
}

function draw_error() {
    echo
    [[ $ERR_MSG ]] && echo $ERR_MSG
    echo
    unset ERR_MSG
}

function redraw_all() {
    clear
    draw_header
    draw_game_field
    draw_error
}

function change_info() {
    INFO=$1
    redraw_all
}

function draw_game_field() {
    echo " ${game_field[0,0]} | ${game_field[0,1]} | ${game_field[0,2]} "
    echo "***********"
    echo " ${game_field[1,0]} | ${game_field[1,1]} | ${game_field[1,2]} "
    echo "***********"
    echo " ${game_field[2,0]} | ${game_field[2,1]} | ${game_field[2,2]} "
}

function set_symbol() {
    if [[ $1 -lt 4 ]] && [[ $1 -gt 0 ]] && \
       [[ $2 -lt 4 ]] && [[ $2 -gt 0 ]]; then
        if [[ ${game_field[$(($1 - 1)),$(($2 - 1))]} == "$EMPTY_CELL" ]]; then
            game_field[$(($1 - 1)),$(($2 - 1))]=$3
            return 0
        else
            ERR_MSG="Not an empty cell!"
            return -1
        fi
    else
        ERR_MSG="Not valid coordinates!" 
        return -1
    fi
}

function check_col_row() {
    declare col row
    for ((i=0; i<3; i++)) do
        row=${game_field[$i,0]}
        col=${game_field[0,$i]}
        for ((j=0; j<3; j++)) do
            if [[ ${game_field[$i,$j]} != $row ]]; then
                row=false
            fi
            if [[ ${game_field[$j,$i]} != $col ]]; then
                col=false
            fi
        done
        [[ $col != false && $col != "$EMPTY_CELL" ]] || \
        [[ $row != false && $row != "$EMPTY_CELL" ]] && break
    done
    if [[ $col != false && $col != "$EMPTY_CELL" ]]; then
        change_info "$col won!"
        clean_all
    fi
    if [[ $row != false && $row != "$EMPTY_CELL" ]]; then
        change_info "$row won!"
        clean_all
    fi
    return -1
}

function check_diags() {
    declare diag1 diag2
    diag1=${game_field[0,0]}
    diag2=${game_field[2,0]}
    for ((i=0; i<3; i++)) do
        if [[ ${game_field[$i,$i]} != $diag1 ]]; then
            diag1=false;
        fi
        if [[ ${game_field[$((2-i)),$i]} != $diag2 ]]; then
            diag2=false;
        fi
    done
    if [[ $diag1 != false && $diag1 != "$EMPTY_CELL" ]]; then
        change_info "$diag1 won!"
        clean_all
    fi
    if [[ $diag2 != false && $diag2 != "$EMPTY_CELL" ]]; then
        change_info "$diag2 won!"
        clean_all
    fi
    return -1
}

function check_win() {

    check_col_row
    check_diags
        
    if [[ ! "${game_field[@]}" =~ "$EMPTY_CELL" ]]; then
        change_info "Draw :)"
        clean_all
    fi
    return -1
}


declare r c
while true; do
    check_win
    if [[ $CUR_MOVE == false ]]; then
        change_info "Opponent's turn"
        read r c < $FIFO
        set_symbol $r $c $OPP
        CUR_MOVE=true
    fi
    change_info 'Your turn'
    check_win
    while true; do
        echo -n '> '
        read r c
        set_symbol $r $c $YOU
        [[ $? -eq 0 ]] && break || redraw_all
    done
    if [[ $CUR_MOVE ]]; then
        change_info "Waiting for opponent..."
        echo $r $c > $FIFO
        CUR_MOVE=false
    fi
done

clean_all
