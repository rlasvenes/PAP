#!/bin/bash

PROG="./run"
SIZE=2048
TILE_SIZE=16
KERNEL="max"
NB_SPIRALE=100
VARIANTS=( "seq" "depend" )
VERBOSE=0

FROM_TS=4
TO_TS=32

function usage {
    echo -e "Usage: ./$0 [-v]"
}

function log () {
    if [[ $VERBOSE -eq 1 ]]; then
        echo -e "$@"
    fi
}

function compute {
    #log "Launching \"$PROG -s $SIZE -k $KERNEL -g $2 -v $1 -a $NB_SPIRALE -n\""
    filename="${SIZE}_${KERNEL}_$2_${NB_SPIRALE}_$1.txt"
    touch $filename
    $PROG -s $SIZE -k $KERNEL -g $2 -v $1 -a $NB_SPIRALE -n > $filename 2>&1 
    runtime=$(cat $filename | cut -d$'\n' -f3)
    rm $filename
    log $runtime
}

function computeVariants {
    plot_filename="gnuplot.txt"
    echo "" > $plot_filename
    touch $plot_filename

    variants_array=("$@")
    for (( i=$FROM_TS; i<=$TO_TS; i*=2 )); do
        echo -n "$i," >> $plot_filename

        for var in "${variants_array[@]}" 
        do
            value=$(compute $var $i )
            echo -e "[ts=$i] runtime = $value [$var]"
            echo -n "$value," >> $plot_filename
        done
        echo -e "" >> $plot_filename
    done
}

if [[ $1 == "-v" ]]; then
    VERBOSE=1
fi

computeVariants "${VARIANTS[@]}"