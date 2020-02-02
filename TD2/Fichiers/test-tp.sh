#!/bin/bash

function usage {
	echo -e "Usage: $0 [nb_villes_from] [nb_villes_to] [seed] [grain] [seq]"
}

function computeTSP {
	for (( i=$1; i<=$2; i++ ))
	do
		#filename="temporary${i}.txt"
		#touch $filename
		#touch failed.txt
		#$(timeout $2 ./userprog/nachos -d s -rs ${i} -x $1 >  $filename) 
		echo "./tsp-main ${i} $3 $4 $5"
		./tsp-main ${i} $3 $4 $5 
		#if [ $? = 0 ]; then
		#	echo "./userprog/nachos -rs ${i} -x $1 success"
		#	#echo "launching \"$1\" with seed \"${i}\" with success !"
		#else
		#	echo "./userprog/nachos -rs ${i} -x $1 timeout"
		#	echo "${i}" >> failed.txt
		#	#echo "launching \"$1\" with seed \"${i}\" timeout !!"
		#fi
		#rm $filename
	done
}

nb_villes_from=$1
nb_villes_to=$2
seed=$3
grain=$4
seq=$5

if [ $# = 0 ]; then
	nb_villes_from=12
	nb_villes_to=15
	seed=1234
	grain=3
	seq="ompfor"
fi	

if [ $1 = "-h" ]; then
	usage
	exit 1
fi


computeTSP $nb_villes_from $nb_villes_to $seed $grain $seq
