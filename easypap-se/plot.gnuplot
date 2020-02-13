#!/usr/bin/env gnuplot -c


set terminal png

set datafile separator comma

set title "Performance test"

set xlabel "Tile size"

set ylabel "Runtime"

set style data line

plot for [i=2:3] filename using i:xtic(1) title columnheader linewidth 4