#!/bin/bash

PROG="./run"
SIZE=2048
TILE_SIZE=16
KERNEL="max"
NB_SPIRALE=100
VARIANTS=( "seq" "depend" )

function usage {
    echo -e "Usage: ./$0 "
}

function compute {
    echo "Launching \"$PROG -s $SIZE -k $KERNEL -g $TILE_SIZE -v $1 -a $NB_SPIRALE -n\""
    filename="${SIZE}_${KERNEL}_${TILE_SIZE}_${NB_SPIRALE}_$1.txt"
    touch $filename
    $PROG -s $SIZE -k $KERNEL -g $TILE_SIZE -v $1 -a $NB_SPIRALE -n > $filename
    runtime=$(cat $filename | cut -d$'\n' -f3)
    echo -e "Time for \"$1\" is $runtime"
}

function computeVariants {
    variants_array=("$@")
    for var in "${variants_array[@]}" 
    do
        echo "Iterate on $var"
        compute $var
    done
}

computeVariants "${VARIANTS[@]}"