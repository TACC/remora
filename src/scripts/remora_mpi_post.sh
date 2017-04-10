rm -f *.pst
outfile="mytemp.pst"
activefile="active.pst"
tablefile="table.pst"

# Step 1 - parse the volume list for calls that actually happened
# Put block of text for task 0 into a file
grep -m 33 -A 33 "Allgather" ./stats.txt  | head -n 33 > ${outfile}

# Step 2 - downselect only those lines with non-zero calls
while read line; do
	calls=$(echo $line | awk '{print $3}')
	active=$( echo " $calls > 0 " | bc )
	if [ "$active" == "1" ]; then
		echo $line >> ${activefile}
	fi
done < ${outfile}

# Step 3 - Now save ALL the cumulative sections to a file
start=()
end=()
start=( $(grep -n "by actual args" ./stats.txt  | awk -F ":" '{print $1}' | cut -d' ' -f1 ) ) 
end=( $(grep -n "by Context" ./stats.txt  | awk -F ":" '{print $1}' | cut -d' ' -f1 ) ) 
proc_start=0; proc_end=0
for i in ${start[@]}; do
    in=$i
    out=${end[$proc_end]}
    range=$((out-in-4))
    grep -A $range "by actual args" ./stats.txt >> ${tablefile}
    proc_end=$((proc_end+1))
done

# Step 4 - For every active call, find the most expensive instance
#while read line; do
#	call=$( echo $line | awk '{print $1}' )
#	times=$( echo $line | awk '{print $3}' )
#	grep --group-separator="" -A $times $call ${tablefile} | awk -v name=$call 'BEGIN {max = 0} {if ($11>max) max=$11;size=$5} END {print name " " size " " max}' 
#done < ${activefile}

# Step 4 - Find the most expensive across these calls
printf "\nProcessing Table (this could take a while) "
while read line; do
	call=$( echo $line | awk '{print $1}' )
	times=$( echo $line | awk '{print $3}' )
	# Select only this MPI call active instances
	grep --group-separator="" -A $times $call ${tablefile} | grep -v $call > ./tmp.pst
	#remove empty lines
	sed -i '/^$/d' ./tmp.pst
	# Loop over each call type and size
	callnum=0
	testOn=1
	while [ $testOn == 1 ]; do
		printf "..."
		while read line2; do 
			callid=$( echo $line2 | awk '{print $1}' )
			if [ $( echo " $callid == ( $callnum + 1 )" | bc ) == 1 ]; then 
				echo $line2 >> ${call}_${callnum}.pst
			fi
		done < ./tmp.pst
		# Find the maximum time spent on this particular instance
		tot_time=$( awk 'BEGIN {max = 0} {if ($11>max) max=$11} END {print max}' ${call}_${callnum}.pst )
		msg_size=$( tail -n 1 ${call}_${callnum}.pst | awk '{print $5}' )
		echo "$call		$msg_size	$tot_time" >> totals.pst
		callnum=$((callnum+1))
		testOn=$( echo "$callnum < $times" | bc )
	done 
	rm -f ./tmp.pst
done < ${activefile}
# sort so most expensive calls are at the top of the file
sort -nr --key=3 ./totals.pst > sorted.pst
printf " Done\n\n"

# Step 5 - Find total time in collectives and apply threshold to define final call file
col_time=$( awk '{sum+=$3} END {print sum}' ./sorted.pst )
threshold=$( echo "scale=3; $col_time * 0.10" | bc )
while read line; do
	call_time=$( echo $line | awk '{print $3}' )
	if [ `echo "$call_time > $threshold" | bc ` == 1 ]; then
		echo $line >> final.pst
	fi
done < sorted.pst

