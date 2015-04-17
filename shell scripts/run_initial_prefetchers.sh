# Runs through all of the given stuff from dpc2sim folder
# Cycles through all prefetchers and all traces with all combinations
# writes to csv file with timestamp as part of name 

# Location of the Prefetchers and traces, must be user specified 
PREFETCHERS="../dpc2sim/example_prefetchers/*.c"
TRACES="../../dpc2simOLD/traces/*.dpc"

# Remove old intermediate files, create new output file
touch currentTrace
rm currentTrace
NOW=$(date -u +"%Y-%m-%dT%H:%M:%S")
NEWFILE="../data/initial_results_"$NOW".csv"


# initial column headers for CSV file 
# Order of flags here should match the order that they are run below
echo "benchmark,prefetcher,noflags,small_llc,low_bandwidth,scramble_loads" >> $NEWFILE


# Need to compile each prefetcher separately 
for prefetcher in $PREFETCHERS
do
  # pulling prefetcher file name from path 
  currentFile=$(basename $prefetcher ".c")
  echo "Compiling $currentFile file..."

  gcc -Wall -o dpc2sim $prefetcher lib/dpc2sim.a

  # Each prefetcher on each trace, with 4 flag options
  for trace in $TRACES
  do
    # create new temp file
    touch currentTrace 

  # Pull trace file name from path 
  traceName=$(basename $trace ".dpc")
  echo "Working on $traceName ... "
  # add to trace, prefetcher labels temp file 
  echo -ne $traceName | sed -e "\$a,"  >> currentTrace
  echo -ne $currentFile | sed -e "\$a," >> currentTrace

  echo "Flag: NONE"
  # Each line: run executable, fetch last number, append, append to current file 
  cat $trace | ./dpc2sim | awk '{w=NF?$NF:w} END{print w}' |  sed -e "\$a," >> currentTrace

  echo  "Flag: small_llc"
  cat  $trace | ./dpc2sim  -small_llc | awk '{w=NF?$NF:w} END{print w}' | sed -e "\$a,">> currentTrace

  echo "Flag: low_bandwidth"
  cat  $trace | ./dpc2sim  -low_bandwidth | awk '{w=NF?$NF:w} END{print w}'| sed -e "\$a," >> currentTrace

  echo  "Flag: scramble_loads"
  
	# No comma on the last line
	cat $trace | ./dpc2sim  -scramble_loads | awk '{w=NF?$NF:w} END{print w}' >> currentTrace

	# Clean up new lines in current trace run, append to end of file for the output file 
  # For each extra newline in currentTrace, increment NR%#?
  awk '{ ORS = (NR%11 ? "" : RS) } 1' currentTrace >> $NEWFILE
  rm currentTrace
done 
done