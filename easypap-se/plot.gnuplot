#!/usr/bin/env gnuplot -c

set terminal png

set datafile separator ","

set title "Performance test"

set xlabel "Tile size"

set ylabel "Runtime (ms)"

set style data line

set autoscale

set logscale x 2


plot filename using 1:2 with lines title columnhead 4, filename using 1:3 with lines title columnhead 