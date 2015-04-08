//
// Data Prefetching Championship Simulator 2
// Original code by: Seth Pugsley, seth.h.pugsley@intel.com
// Modified by Sebastian Krupa, Jean Sung, Andrew Fishberg, Josef Spjut
// Paper #7

/*

  This file describes an Instruction Pointer-based (Program Counter-based) stride prefetcher.  
  The prefetcher detects stride patterns coming from the same IP, and then 
  prefetches additional cache lines.

  Prefetches are issued into the L2 or LLC depending on L2 MSHR occupancy.

 */

#include <stdio.h>
#include "../inc/prefetcher.h"

#define IP_TRACKER_COUNT 128
#define PREFETCH_DEGREE_LOW 1
#define PREFETCH_DEGREE 4
#define PREFETCH_DEGREE_HIGH 8
#define THRESHOLD 30
#define STREAM_DEPTH 2
#define COMMON_COUNT 70
#define UNCOMMON 20

typedef struct ip_tracker
{
  // the IP we're tracking
  unsigned long long int ip;

  // the last address accessed by this IP
  // this can be 58 bits since the bottom 6 bits are unused because of the block offset
  unsigned long long int last_addr;
  
  // the stride between the last two addresses accessed by this IP
  // this will be 6 bits since the bottom 6 bits are unused because of the block offset
  // and the top 20 bits are unneccessary since we don't prefetch across pages
  long int last_stride;

  //stream is never larger than 5 bits (-8 to 8)
  long int stream;

  // use LRU to evict old IP trackers
  //this can be a 6 bit number since there are only 128 entries
  unsigned long int lru_cycle;
    
  // the number of times this instruction has missed
  unsigned long int miss;

  // the number of times this instruction has been called
  unsigned long int cycle_num;
} ip_tracker_t;

ip_tracker_t trackers[IP_TRACKER_COUNT];

// this struct keeps track of the miss rate of common instructions only
// this struct is not strictly necessary but helps with run time
typedef struct common_counter {
    unsigned long int miss;
    unsigned long int cycle_num;
} common_counter_t;

common_counter_t MPC[COMMON_COUNT];

void l2_prefetcher_initialize(int cpu_num)
{
  printf("IP-based Stride Prefetcher\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);
	
  //make sure to initialize all variables within struct to 0
  int i;
  for(i=0; i<IP_TRACKER_COUNT; i++)
    {
        trackers[i].ip = 0;
        trackers[i].last_addr = 0;
        trackers[i].last_stride = 0;
        trackers[i].lru_cycle = 0;
        trackers[i].stream = 0;
        trackers[i].miss = 0;
        trackers[i].cycle_num = 0;
        if(i < COMMON_COUNT) {
            MPC[i].miss = 0;
            MPC[i].cycle_num = 0;
        }
    }
}

void l2_prefetcher_operate(int cpu_num, unsigned long long int addr, unsigned long long int ip, int cache_hit)
{
  // uncomment this line to see all the information available to make prefetch decisions
  //printf("(%lld 0x%llx 0x%llx %d %d %d) ", get_current_cycle(0), addr, ip, cache_hit, get_l2_read_queue_occupancy(0), get_l2_mshr_occupancy(0));

  // check for a tracker hit
  int tracker_index = -1;

  //see if currently called instruction is already being tracked
  int i;
  for(i=0; i<IP_TRACKER_COUNT; i++)
    {
      if(trackers[i].ip == ip)
      {
        trackers[i].lru_cycle = get_current_cycle(0);
        tracker_index = i;
        break;
      }
    }

  if(tracker_index == -1)
    {
      // this is a new IP that doesn't have a tracker yet, so allocate one
      int lru_index=0;
      unsigned long long int lru_cycle = trackers[lru_index].lru_cycle;
      int i;
      // evict least recently used instruction
      for(i=0; i<IP_TRACKER_COUNT; i++)
        {
          if(trackers[i].lru_cycle < lru_cycle)
            {
              lru_index = i;
              lru_cycle = trackers[lru_index].lru_cycle;
            }
        }

      tracker_index = lru_index;

      // reset the old tracker
      trackers[tracker_index].ip = ip;
      trackers[tracker_index].last_addr = addr;
      trackers[tracker_index].last_stride = 0;
      trackers[tracker_index].lru_cycle = get_current_cycle(0);
      trackers[tracker_index].stream = 0;
        trackers[tracker_index].miss = 0;
        trackers[tracker_index].cycle_num = 0;

      return;
    }

    //increment cycles and misses if necessary
    trackers[tracker_index].miss += (1-cache_hit);
    trackers[tracker_index].cycle_num += 1;

    // temporary variables for bubble sort
    int temp, temp2;
   
    // bubble sort by cycles then keep top 50 elements
    for(temp = 0; temp<IP_TRACKER_COUNT; temp++) {
    	for(temp2 = 0; temp2<(IP_TRACKER_COUNT-temp-1); temp2++) {
            if(trackers[temp2].cycle_num > trackers[temp2 + 1].cycle_num) {
                ip_tracker_t temp_struct = trackers[temp2];
    			trackers[temp2] = trackers[temp2+1];
    			trackers[temp2+1] = temp_struct;
    			
    			//sorting will mess up tracker_index, have to keep it updated with current position
    			if(temp2 == tracker_index){ 
    				tracker_index = temp2+1;
    			} else if((temp2+1) == tracker_index) {
    				tracker_index = temp2;
    			}
            }
        }
    }
    
    //put 50 most common in MPC array
    for(temp = UNCOMMON; temp<COMMON_COUNT; temp++) {
        MPC[temp].miss = trackers[IP_TRACKER_COUNT-COMMON_COUNT+temp].miss;
        MPC[temp].cycle_num = trackers[IP_TRACKER_COUNT-COMMON_COUNT+temp].cycle_num;
    }

    //put 20 from the less common in (evenly spaced)
    for(temp = 0; temp<UNCOMMON; temp++) {
        MPC[temp].miss = trackers[(int)((float)temp*(((float)COMMON_COUNT-UNCOMMON)/(float)UNCOMMON))].miss;
        MPC[temp].cycle_num = trackers[(int)((float)temp*(((float)COMMON_COUNT-UNCOMMON)/(float)UNCOMMON))].cycle_num;
    }
    
    //the least common element in the MPC array
    long int least_common_common = MPC[UNCOMMON].cycle_num;
    
    //sort MPC array by MPC
    for(temp = 0; temp<COMMON_COUNT; temp++) {
    	for(temp2 = 0; temp2<(COMMON_COUNT-temp); temp2++) {
    		if( ((float)MPC[temp2].miss/(float)MPC[temp2].cycle_num) > ((float)MPC[temp2+1].miss/(float)MPC[temp2+1].cycle_num)) {
    			common_counter_t temp_struct = MPC[temp2];
    			MPC[temp2] = MPC[temp2+1];
    			MPC[temp2+1] = temp_struct;
    		}
    	}
    }

    
    int percent = THRESHOLD;
    int prefetch_low = PREFETCH_DEGREE_LOW;
    int prefetch_standard = PREFETCH_DEGREE;
    int prefetch_high = PREFETCH_DEGREE_HIGH;
    
    int prefetch_degree_used;
    
    // convert the percentage threshold int an index
    int thresh;
    thresh = (COMMON_COUNT - 1)*((float)percent/100.0);
    
    //rare instruction
    if(trackers[tracker_index].cycle_num < least_common_common) {
         //if doing well
         if((float)trackers[tracker_index].miss/(float)trackers[tracker_index].cycle_num < (float)trackers[thresh].miss/(float)trackers[thresh].cycle_num)
            prefetch_degree_used = prefetch_low;
         else
            prefetch_degree_used = prefetch_standard;
    } else { //common instruction
        //if doing well
        if((float)trackers[tracker_index].miss/(float)trackers[tracker_index].cycle_num < (float)trackers[thresh].miss/(float)trackers[thresh].cycle_num)
            prefetch_degree_used = prefetch_standard;
         else
            prefetch_degree_used = prefetch_high;
    }
  
  //calculate the stride
  long long int stride = 0;
  if(addr > trackers[tracker_index].last_addr)
    {
      stride = addr - trackers[tracker_index].last_addr;
    }
  else
    {
      stride = trackers[tracker_index].last_addr - addr;
      stride *= -1;
    }
  
  // don't do anything if we somehow saw the same address twice in a row
  if(stride == 0)
    {
      return;
    }
 
  // only do any prefetching if there's a pattern of seeing the same
  // stride more than once
  if(stride == trackers[tracker_index].last_stride)
    {
      int i, j;
      int k = 0;
	
      // if there is a stream along with the stride	first stream up to 2 lines
      // of data then stride, follow this up with more stream
      if(trackers[tracker_index].stream > 0) {
	      for(j=0, k=0; k<=prefetch_degree_used; j++) {

			for(i=0; i<=trackers[tracker_index].stream; i++) {
				k++;
				
                //make sure we are not prefetching too much
				if(k>prefetch_degree_used)
				  break;

                //make sure we are not streaming too much
                if(i>STREAM_DEPTH)
                  break;

                //calculate the adress we want to prefetch
                //j is the number of strides we have made
                //i is the number of streams
				unsigned long long int pf_address = addr + ((j+1)*stride);
				pf_address = ((pf_address>>6) + i)<<6;

				// only issue a prefetch if the prefetch address is in the same 4 KB page 
				// as the current demand access address
				if((pf_address>>12) != (addr>>12))
				  {
				    break;
				  }

				// check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
				if(get_l2_mshr_occupancy(0) < 8) {
				    l2_prefetch_line(0, addr, pf_address, FILL_L2);
				}else{
				    l2_prefetch_line(0, addr, pf_address, FILL_LLC);
				  }
				
			  }
			}
	 } else { //stream <= 0
	    for(j=0, k=0; k<=prefetch_degree_used; j++) {
			for(i=0; i>=trackers[tracker_index].stream; i--)
			  {
				k++;
				if(k>prefetch_degree_used)
				  break;
                if(i<-STREAM_DEPTH)
                  break;
				unsigned long long int pf_address = addr + ((j+1)*stride);
				pf_address = ((pf_address>>6) + i)<<6;

				// only issue a prefetch if the prefetch address is in the same 4 KB page 
				// as the current demand access address
				if((pf_address>>12) != (addr>>12))
				  {
				    break;
				  }

				// check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
				if(get_l2_mshr_occupancy(0) < 8)
				  {
				    l2_prefetch_line(0, addr, pf_address, FILL_L2);
				  }
				else
				  {
				    l2_prefetch_line(0, addr, pf_address, FILL_LLC);
				  }
				
			  }
		}
	}

    // there is no stride pattern, so stream only
  } else if((addr>>6) == ((trackers[tracker_index].last_addr>>6) + 1)) {
        // dont want to override stride if streaming
        stride = trackers[tracker_index].last_stride; 
        
        //if it was going in the other direction earlier, reset it
        if(trackers[tracker_index].stream < 0)
            trackers[tracker_index].stream = 0;
        trackers[tracker_index].stream += 1;     

	    for(i=0; i<trackers[tracker_index].stream; i++) {
		    unsigned long long int pf_address = addr + (i*64);

                if(i>prefetch_degree_used)
				  break;

                // only issue a prefetch if the prefetch address is in the same 4 KB page 
                // as the current demand access address
                if((pf_address>>12) != (addr>>12))
                  {
                    break;
                  }

                // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
                if(get_l2_mshr_occupancy(0) < 8)
                  {
                    l2_prefetch_line(0, addr, pf_address, FILL_L2);
                  }
                else
                  {
                    l2_prefetch_line(0, addr, pf_address, FILL_LLC);
                  }
	    }
    } else if((addr>>6) == ((trackers[tracker_index].last_addr>>6) - 1)) {
        //dont want to override stride if streaming
        stride = trackers[tracker_index].last_stride; 
        
        if(trackers[tracker_index].stream > 0)
            trackers[tracker_index].stream = 0;
        trackers[tracker_index].stream -= 1;
	    
        for(i=0; i>trackers[tracker_index].stream; i--) {
		    unsigned long long int pf_address = addr + (i*64);

                if((-1*i)>prefetch_degree_used)
				  break;

                // only issue a prefetch if the prefetch address is in the same 4 KB page 
                // as the current demand access address
                if((pf_address>>12) != (addr>>12))
                  {
                    break;
                  }

                // check the MSHR occupancy to decide if we're going to prefetch to the L2 or LLC
                if(get_l2_mshr_occupancy(0) < 8)
                  {
                    l2_prefetch_line(0, addr, pf_address, FILL_L2);
                  }
                else
                  {
                    l2_prefetch_line(0, addr, pf_address, FILL_LLC);
                  }
	    }
    }
  
  //store relevant values  
  trackers[tracker_index].last_addr = addr;
  trackers[tracker_index].last_stride = stride;
}

void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr)
{
  // uncomment this line to see the information available to you when there is a cache fill event
  //printf("0x%llx %d %d %d 0x%llx\n", addr, set, way, prefetch, evicted_addr);
}
