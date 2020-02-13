#!/bin/bash

PROG="./run"
SIZE=2048 
TILE_SIZE=16
KERNEL="max"
NB_SPIRALE=100
VARIANTS=( "seq" "depend" "tiled" "omp")
VERBOSE=0

FROM_TS=4 # start index of tile size
TO_TS=32    

PLOT_FILENAME="data.csv"
OUT="out.png"

function usage () {
    echo -e "Usage: ./$0 [-v]"
}

function log () {
    if [[ $VERBOSE -eq 1 ]]; then
        echo -e "$@"
    fi
}

function join_by () { 
    local d=$1
    shift
    echo -n "$1"
    shift
    printf "%s" "${@/#/$d}"
}

function compute () {
    filename="${SIZE}_${KERNEL}_$2_${NB_SPIRALE}_$1.txt"
    touch $filename
    $PROG -s $SIZE -k $KERNEL -g $2 -v $1 -a $NB_SPIRALE -n > $filename 2>&1 
    runtime=$(cat $filename | cut -d$'\n' -f3)
    rm $filename
    log $runtime
}

function create_empty_csv_file () {
    if [[ $# -eq 1 ]]; then
        $PLOT_FILENAME=$1
    fi
    touch $PLOT_FILENAME
    echo "" > $PLOT_FILENAME

    headers=("$@")
    join_by ',' "${headers[@]}" > $PLOT_FILENAME
    echo "" > $PLOT_FILENAME
}

function plot_perf () {
    gnuplot -e "filename='$1'" plot.gnuplot
}

function compute_variants {
    create_empty_csv_file "Tile size" $@

    variants_array=("$@")
    last_index=$(( ${#variants_array[*]} - 1 ))
    last=${variants_array[$last_index]}
    for (( i=$FROM_TS; i<=$TO_TS; i*=2 )); do
        echo -n "$i," >> $PLOT_FILENAME

        for var in "${variants_array[@]}" 
        do
            value=$(compute $var $i )
            log "[ts=$i] runtime = $value [$var]"
            if [[ $var == $last ]]; then
                echo -n "$value" >> $PLOT_FILENAME    
            else
                echo -n "$value," >> $PLOT_FILENAME
            fi
        done
        echo -e "" >> $PLOT_FILENAME
    done
}

if [[ $1 == "-v" ]]; then
    VERBOSE=1
fi

compute_variants "${VARIANTS[@]}"
plot_perf $PLOT_FILENAME > $OUT
