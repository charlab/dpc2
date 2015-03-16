# INSTRUCTIONS: 
# (1) Must supply INITIAL_FILE that holds the entire path to the original file 
# to append the results to.
# (2) Must be run with the path to the given prefetcher as the only command line
# argument 


# Location of the Prefetchers and traces 
INITIAL_FILE="../data/initial_results.csv"

# THIS is THE LOCATION OF THE OLD TRACES, NOT COMMITED ON GITHUB
TRACES="../../dpc2simOLD/traces/*.dpc"
TEST_PREFETCHER_LOCATION=$1
echo $1


# Remove old intermediate files, create new output file
touch currentTrace
rm currentTrace

# Current file has date + prefetcher name, iso format 
TEST_PREFETCHER_NAME=$(basename $TEST_PREFETCHER_LOCATION ".c")
NOW=$(date -u +"%Y-%m-%dT%H:%M:%S")
NEWFILE="../data/"$TEST_PREFETCHER_NAME"_"$NOW".csv"
cp $INITIAL_FILE $NEWFILE

# Start with a newline 
printf "" >> $NEWFILE

gcc -Wall -o dpc2sim $TEST_PREFETCHER_LOCATION ../lib/dpc2sim.a

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
  echo -ne $TEST_PREFETCHER_NAME | sed -e "\$a," >> currentTrace

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