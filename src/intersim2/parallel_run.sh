#!/bin/sh

# Script to run specfied booksim configuraitons in parallel using parallel.
# This script runs the run_booksim.sh script for each configuration.
# This is for booksim standalone simulations

arg_file=arg_file
if [ -f $arg_file ]; then
	rm $arg_file
fi

topologies=("3dmesh" "vb-3dmesh" "2asym" "4sym")
traffic_patterns=("uniform" "hotspot")
write_ratios=("0.1" "0.3" "0.5")

#topologies=("3dmesh" "vb-3dmesh")
#traffic_patterns=("uniform")
#write_ratios=("0.1")

for topology in ${topologies[@]}
do
	for traffic in ${traffic_patterns[@]}
	do
		for ratio in ${write_ratios[@]}
		do 
			input="-n ${topology} -t ${traffic} -w ${ratio}"
			session_name="${topology}_${traffic}_${ratio}"
			echo "${session_name}:${input}" >> $arg_file
		done
	done
done

cat ${arg_file}| parallel --colsep ':' screen -mdS {1} run_booksim.sh {2}
rm $arg_file
