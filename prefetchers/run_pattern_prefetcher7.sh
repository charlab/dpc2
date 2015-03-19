# INSTRUCTIONS: 
# (1) Must supply INITIAL_FILE that holds the entire path to the original file 
# to append the results to.
# (2) Must be run with the path to the given prefetcher as the only command line
# argument 


# Location of the Prefetchers and traces 
#INITIAL_FILE="../data/initial_results.csv"

CONFIG_FILE="all_config7.csv"

# THIS is THE LOCATION OF THE OLD TRACES, NOT COMMITED ON GITHUB
TRACES="../../dpc2simOLD/traces/*.dpc"
TEST_PREFETCHER_LOCATION=$1
echo "working on prefetcher: "$1


# Remove old intermediate files, create new output file
touch currentTrace7
rm currentTrace7

# Current file has date + prefetcher name, iso format 
TEST_PREFETCHER_NAME=$(basename $TEST_PREFETCHER_LOCATION ".c")
NOW=$(date -u +"%Y-%m-%dT%H:%M:%S")
NEWFILE="../data/"$TEST_PREFETCHER_NAME"_"$NOW".csv"
#cp $INITIAL_FILE $NEWFILE

# initial column headers for CSV file 
# Order of flags here should match the order that they are run below
echo "benchmark,prefetcher,noflags,small_llc,low_bandwidth,scramble_loads,sum,threshold,low_degree,high_degree" >> $NEWFILE

gcc -Wall -o dpc2sim7 $TEST_PREFETCHER_LOCATION ../lib/dpc2sim.a

# each configuration setting 
while read currentLine;
  do 
  # Pipe current line into the test_line7.csv file
  rm test_line7.csv
  echo $currentLine > test_line7.csv
  echo "current configuration: "$currentLine

  # Parse current line's configurations in to variables 
  IFS=',' read -a currentLineElements <<< "$currentLine"
  THRESHOLD="${currentLineElements[0]}"
  LOW_DEGREE="${currentLineElements[1]}"
  HIGH_DEGREE="${currentLineElements[2]}"



  # Each prefetcher on each trace, with 4 flag options
  for trace in $TRACES
    do
    # create new temp file
    touch currentTrace7 

    # Pull trace file name from path 
    traceName=$(basename $trace ".dpc")
    echo "Working on $traceName ... "
    # add to trace, prefetcher labels temp file 
    echo -ne $traceName | sed -e "\$a,"  >> currentTrace7
    echo -ne $TEST_PREFETCHER_NAME | sed -e "\$a," >> currentTrace7

    echo "Flag: NONE"
    # Each line: run executable, fetch last number, append, append to current file 
    cat $trace | ./dpc2sim7 | awk '{w=NF?$NF:w} END{print w}' |  sed -e "\$a," >> currentTrace7

    echo  "Flag: small_llc"
    cat  $trace | ./dpc2sim7  -small_llc | awk '{w=NF?$NF:w} END{print w}' | sed -e "\$a,">> currentTrace7

    echo "Flag: low_bandwidth"
    cat  $trace | ./dpc2sim7  -low_bandwidth | awk '{w=NF?$NF:w} END{print w}'| sed -e "\$a," >> currentTrace7

    echo  "Flag: scramble_loads"
    cat $trace | ./dpc2sim7  -scramble_loads | awk '{w=NF?$NF:w} END{print w}'| sed -e "\$a," >> currentTrace7

    # Calculate the sum
    # Takes the currentTrace7 file, prints even lines (to get rid of commas)
    # removes top line (name of the trace)
    # sums up all of the numbers 
    # adds comma!
    cat currentTrace7 | awk 'NR % 2' | tail -n +2  | awk '{s+=$1} END {print s}' | sed -e "\$a," >> currentTrace7


    # Write the current settings to the csv file 
    echo -ne $THRESHOLD | sed -e "\$a," >> currentTrace7
    echo -ne $LOW_DEGREE | sed -e "\$a," >> currentTrace7
    # No comma on the last line
    echo -ne $HIGH_DEGREE >> currentTrace7


    # Clean up new lines in current trace run, append to end of file for the output file 
    # For each extra newline in currentTrace7, increment NR%#?
    #Number of columns + 8
    awk '{ ORS = (NR%19? "" : RS) } 1' currentTrace7 >> $NEWFILE
    rm currentTrace7
    done 
done <$CONFIG_FILE
