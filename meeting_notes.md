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
