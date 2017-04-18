#!/bin/sh

#TODO write instructions

# it can be run in three ways 
# 1 string input
# configuration parameters
# without paramters


#booksim source path
exec_path="/home/kaya1/jgardea/GPGPU-SIM/gpgpu-sim/src/intersim2/"

#default executable and configuration file
sim="$exec_path/booksim"
configfile="$exec_path/booksim_config"

sim_run="${sim} ${configfile}"

# This set up is for parallel 
if [ "$2" == "" ] && [ "$1" != "" ]; then
	set -- $1
fi

args=($@)

net_topo=""
traffic=""
write_rate=""
i=0

for arg in "${args[@]}"
do 
	arg=${args[${i}]}

	if [ "$arg" == "-n" ]; then
		i=$(($i + 1))    
		net_topo=${args[${i}]}
	
	elif [ "$arg" == "-t" ]; then
		i=$(($i + 1))    
		traffic=${args[${i}]}
		
	elif [ "$arg" == "-w" ]; then
		i=$(($i + 1))    
		write_rate=${args[${i}]}
	fi
	i=$(($i + 1))
done

#This mode runs booksim with only with the confiruation file
if [ "$net_topo" == "" ] && [ "$traffic" == "" ] && [ "$write_rate" == "" ]; then
	${sim_run} 
	exit
fi

#Configuration Strings

str_net=""
str_traffic=""
str_wr_rate=""
str_pckt_sizes=""
long_packet_flits=""

#Network topology configuration

if [ "$net_topo" == "3dmesh" ]; then
	topo="topology=mesh"
	k="k=4"; n="n=3"; s="s=1";
	router="router=iq"
	flit_size="flit_size=256" 
	str_net="${topo} ${k} ${n} ${s} ${router} ${flit_size}"

elif [ "$net_topo" == "2asym" ]; then
	subnets="subnets=2"
	long_packet_flits=6
	subnet_selection="subnet_selection=3"
	flit_size="flit_size=64"
	str_net="${subnets} ${subnet_selection} ${flit_size}"

elif [ "$net_topo" == "4sym" ]; then
	subnets="subnets=4"
	long_packet_flits=17
	subnet_selection="subnet_selection=1"
	flit_size="flit_size=64"
	str_net="${subnets} ${subnet_selection} ${flit_size}"

elif [ "$net_topo" == "vb-3dmesh" ] || [ "$net_topo" == "" ]; then
	str_net=""

else 
	echo "Network topology ${net_topo} not supported in this script"
	exit
fi

#Traffic Configuration

if [ "$traffic" == "uniform" ]; then
	restrict_srcs="restrict_sources=0"
	str_traffic="traffic=uniform ${restrict_srcs}"
	
elif [ "$traffic" == "hotspot" ]; then
	restrict_srcs="restrict_sources=1"
	str_traffic="traffic=hotspot({pattern,0}) ${restrict_srcs}"

elif [ "$traffic" == "" ]; then
	str_traffic=""

else
	echo "Traffic Pattern ${traffic} not supported in this script"
	exit
fi

#Packet Size

if [ "$long_packet_flits" != "" ]; then
	write_reqs="write_request_size=${long_packet_flits}"
	read_reply="read_reply_size=${long_packet_flits}"
	str_pckt_sizes="${write_reqs} ${read_reply}"
fi

#Write Rate

if [ "$write_rate" != "" ]; then
   str_wr_rate="write_fraction=${write_rate}" 
fi

param_overwrite="${str_net} ${str_traffic} ${str_wr_rate} ${str_pckt_sizes}"
echo $param_overwrite
sim_run="${sim} ${configfile} ${param_overwrite}"

# Result folders set up

date=`date "+%b_%d_%H"`

result_dir="${BOOKSIM_RESULTS}/${traffic}/${date}"
log_path="${result_dir}/logs"
power_path="${result_dir}/power"
log_name="${write_rate}_${traffic}_${net_topo}"

if [ ! -d "$result_dir" ]; then
	mkdir $result_dir
fi

if [ ! -d "$log_path" ]; then
	mkdir "$log_path" 
fi 

if [ ! -d "$power_path" ]; then
	mkdir "$power_path"
fi

# Log files set up
zero_load_log="${log_path}/zero_load_${log_name}.log"
sat_load_log="${log_path}/sat_load_${log_name}.log"
temp_log=$log_name
# result files set up
zero_power="${power_path}/zero_${traffic}_${write_rate}.dat"
sat_power="${power_path}/sat_${traffic}_${write_rate}.dat"
result="${result_dir}/${log_name}.dat"

initial_step=0.02
zero_load_inj=0.0025

if [ -f $result ]; then
	rm $result
fi

echo "SWEEP: Determining zero-load latency..."

${sim_run} injection_rate=${zero_load_inj} | tee ${zero_load_log}

zero_load_lat=`grep "results:" ${zero_load_log} | cut -d , -f 2`
power=`grep "power_values:" ${zero_load_log} | cut -d , -f 2`

if [ "${zero_load_lat}" = "" ]
then
	echo "SWEEP: Simulation run failed."
	echo "SWEEP: Aborting."
	exit
fi

echo "SWEEP: Simulation run succeeded."
echo "SWEEP: Zero-load latency is ${zero_load_lat}."

echo "${zero_load_inj}	${zero_load_lat}" >> ${result}
echo "${power}	${net_topo}" >> $zero_power

step=${initial_step}
old_inj=0.0
inj=${step}

echo "SWEEP: Sweeping with initial step size ${step}"

while true
do
	echo "SWEEP: Simulating for injection rate ${inj}..."
	
	${sim_run} injection_rate=${inj} | tee ${temp_log} 
	lat=`grep "results:" ${temp_log} | cut -d , -f 2`

	if [ "${lat}" == "" ]
	then
		rm ${temp_log}
		echo "SWEEP: Simulation run failed."
		break
	else
		cp $temp_log $sat_load_log
		echo "${inj}	${lat}" >> ${result}
	fi
  
	rm ${temp_log}

	echo "SWEEP: Simulation run succeeded."
	echo "SWEEP: Latency is ${lat}."
	
	old_inj=${inj}
	inj="`awk "BEGIN{ print ${inj} + ${step} }"`"
done

echo "$inj	500" >> $result

power=`grep "power_values:" ${sat_load_log} | cut -d , -f 2`
echo "${power}	${net_topo}" >> $sat_power
