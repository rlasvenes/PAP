#!/bin/bash

function usage {
    echo -e "Usage: ./$0 [size] [tile_size] [variant] [nb_spirale]"
}

function testSpirale_maxKernel {
    echo "Launching \"./run -s $1 -k max -g $2 -v $3 -a $4\""
    ./run -s $1 -k max -g $2 -v $3 -a $4 2>1 /dev/null
}

function computeVariants {
    variants_array=("$@")
    for var in "${variants_array[@]}" 
    do
        echo "Iterate on $var"
    done
}

size=2048
tile_size=16
variant="seq"
nb_spirale=100

variants_to_test=( "seq" "depend" )

testSpirale_maxKernel $size $tile_size $variant $nb_spirale
computeVariants "${variants_to_test[@]}"