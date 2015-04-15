## Meeting Notes /Task List 

### Tuesday March 3rd

* To dos for Friday 

| Person | Task |
|:-------|:-----|
| Prof Spjut | Figure out how much memory we are allowed to use, actual rules of the prefetcher we create |
| Jean | fix script's weird newline bug, enhance it so that it appends new prefetcher result to example prefetchers <br> figure out ampm fetcher|
| Sebastian | continue trying to understand ampm prefetcher, come with new concept or implementation |
|  Andrew | more progress on traces, come up with new traces, write up how to use pin tool for the team |


### Friday, March 6th 
| Person | Task |
|:-------|:-----|
| Jean | finish the scripts and organize the github repo <br> read the papers on the prefetcher <br> look at how the am pm prefetcher works |
| Andrew | find out the working set size of the initial traces <br> use python script to find the number of unique addresses <br> work on the NAS benchmarks (using single threaded mode) |
|  Sebastian | work on attempt coded example |


## Tuesday, March 10th
| Person | Task |
|:-------|:-----|
| Jean | rerun initial data, pattern prefetcher <br> add assumptions that the script needs (i.e. you need to have a local copy of the trace, and you need to specify the location) <br> change gitignore file to include the lib files  |
|  Sebastian | work on MPC (with Jean) |
| Andrew | continue tasks from above, ??? |

* How do we know performance? 
* usual metric instruction per cycle (have the cycle but we don't know how to get the instruction) 
* we can also check to see if what we fetch is being used
* fill ll2, llc (last level cache also level 3 cache)
* idea: calculate a misses per cycle rate
* an directly access this value, and use it as proxy for the competition IPC
* the logic is if MPC is high then give it higher priority because we are not prefetching enough 

<!--can check MSHR (where to put the line) 
hit under miss 
optimization for cache
how many misses you can have at a time-->


## Thursday, March 12th (Jean and Sebastian)

* We met to work on the MPC and were able to implement a prototype version pretty quickly
* Notice that the IPC value is very trace dependent, for example: a threshold of 0.92 gave us IPC of 0.34 for gcc and 3.44 for GEMS
* next step: want to try many experiments
	* vary prefetch low/high degree (5, 10)
	* the threshold percent
<!--translates to misses per cycle 0.92 in gcc case
-->

## Friday, March 13th (team meeting)
* based on the results from yesterday, we are moving forward with experiments (good spring break work)
* we want to vary: high degree/low degree of prefetching and the threshold
* data points for varying the low degree (the number represents the amount relegated to the lower degree of prefetching): 0, 25, 50, 60, 70, 80, 90, 100
* data points for the pairs of prefetching: 1 and 3, 4 and 7, 8 and 10,
* we are using these few data points at first to make sure we can finish running during spring break and also gives us a chance to zoom in on the results and then decide where the optimal space/solution might be (we are NOT expecting weird behavior where the optimal solution is hidden among really bad data points)

| Person | Task |
|:-------|:-----|
| Jean | add extra columns to the results csv for the pattern prefetcher (threshold percent, low prefetch degree, high prefetch degree),  more work on shell scripts, add a sum column for the initial results |
|  Sebastian | update pattern prefetch c file  |
| Josef | initial draft of paper |

## Tuesday, March 24th (team meeting)

* next step is to try more runs
	* saving 50 of  most common instructions 
	* do more close analysis on the lowering range for the percentage (0-50)
	* instead: if you are doing poorly prefetch more
	* now: if you are common + doing well: prefetch less
	* if you are rare + doing well: prefetch even less
	* if you are common + doing poorly: prefetch more 
	* if you are rare + doing poorly: prefetch less or even less 
* Final Push timeline: 
	*  for this Friday: finishing running prefetcher on existing traces
	* Friday meeting: convene and do data analysis to find the optimal solution (data analysis and stitching)
	* this weekend: run prefetcher on Andrew's traces
	* next week (last week): paper writing! 


| Person | Task |
|:-------|:-----|
| Jean | add in new parameter for script, stitch together original data, review code for bugs and style   |
|  Sebastian | update pattern prefetch c file for new parameters, run the prefetcher   |
| Andrew | more traces, review code for bugs and style| 
| Josef | draft of paper |



## Friday, March 27th

* At this meeting, we realized we did not end up having the data run yet
* Sebastian did update the pattern prefetcher
* Andrew came up with several new traces

| Person | Task |
|:-------|:-----|
| Jean | update the scripts, update the test_line files for 8 piecewise, run the initial ones again |
|  Sebastian | run the actual tests  |
| Andrew | run the traces on all of the prefetchers|

* every needs to do debugging, preferably today because we are going to run tests soon
* looking forward to the final paper: 

| Section | Person |
|:--------| :-------|
| Background | Andrew |
| Prefetcher | Sebastian |
| Traces | Andrew |
| Results | Jean | 


Top priority: make sure the code is bug free by 3am tonight (when the tests will start running)

