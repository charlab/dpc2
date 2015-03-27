//
// Data Prefetching Championship Simulator 2
// Seth Pugsley, seth.h.pugsley@intel.com
//

/*

  This file describes an Instruction Pointer-based (Program Counter-based) stride prefetcher.  
  The prefetcher detects stride patterns coming from the same IP, and then 
  prefetches additional cache lines.

  Prefetches are issued into the L2 or LLC depending on L2 MSHR occupancy.

 */

#include <stdio.h>
#include "../inc/prefetcher.h"

#define IP_TRACKER_COUNT 128
#define PREFETCH_DEGREE 5
#define PREFETCH_DEGREE_HIGH 10
#define THRESHOLD 0.92
#define STREAM_DEPTH 2
#define COMMON_COUNT 70
#define UNCOMMON 10

typedef struct ip_tracker
{
  // the IP we're tracking
  unsigned long long int ip;

  // the last address accessed by this IP
  unsigned long long int last_addr;
  // the stride between the last two addresses accessed by this IP
  long int last_stride;

  long int stream;

  long int stream_stride;
  long int last_stream_stride;

  // use LRU to evict old IP trackers
  unsigned long long int lru_cycle;

    unsigned long int miss;
    unsigned long long int cycle_num;
} ip_tracker_t;

ip_tracker_t trackers[IP_TRACKER_COUNT];

typedef struct common_counter {
    unsigned long int miss;
    unsigned long long int cycle_num;
} common_counter_t;

common_counter_t MPC[COMMON_COUNT];

void l2_prefetcher_initialize(int cpu_num)
{
  printf("IP-based Stride Prefetcher\n");
  // you can inspect these knob values from your code to see which configuration you're runnig in
  printf("Knobs visible from prefetcher: %d %d %d\n", knob_scramble_loads, knob_small_llc, knob_low_bandwidth);
	
  int i;
  for(i=0; i<IP_TRACKER_COUNT; i++)
    {
        trackers[i].ip = 0;
        trackers[i].last_addr = 0;
        trackers[i].last_stride = 0;
        trackers[i].lru_cycle = 0;
        trackers[i].stream = 0;
        trackers[i].stream_stride = 0;
        trackers[i].last_stream_stride = 0;
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
      trackers[tracker_index].stream_stride = 0;
      trackers[tracker_index].last_stream_stride = 0;
        trackers[tracker_index].miss = 0;
        trackers[tracker_index].cycle_num = 0;

      return;
    }
    //fprintf(stderr, "%ld\n",trackers[tracker_index].stream);
  // calculate the stride between the current address and the last address
  // this bit appears overly complicated because we're calculating
  // differences between unsigned address variables
    trackers[tracker_index].miss += (1-cache_hit);
    trackers[tracker_index].cycle_num += 1;
    
    //printf("MISSES_PER_CYCLE: %f\n", MPC);
    //printf("Misses: %ld\nCycles: %lld\n\n", trackers[tracker_index].miss,trackers[tracker_index].cycle_num);

    
    int temp, temp2;
   
    //really bad bubble sort algorithm, but we can't write other functions so this is easiest to do
    //sort by cycles then keep top 50 elements
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
    
    //put 30 most common in MPC array
    for(temp = UNCOMMON; temp<COMMON_COUNT; temp++) {
        MPC[temp].miss = trackers[IP_TRACKER_COUNT-COMMON_COUNT+temp].miss;
        MPC[temp].cycle_num = trackers[IP_TRACKER_COUNT-COMMON_COUNT+temp].cycle_num;
    }
    //put 20 from the less common in (evenly spaced)
    for(temp = 0; temp<UNCOMMON; temp++) {
        MPC[temp].miss = trackers[(int)((float)temp*(((float)COMMON_COUNT-UNCOMMON)/(float)UNCOMMON))].miss;
        MPC[temp].cycle_num = trackers[(int)((float)temp*(((float)COMMON_COUNT-UNCOMMON)/(float)UNCOMMON))].cycle_num;
    }
    
    //the least common element in the MPC array;
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

    
    int percent = 50;
    int prefetch_low = 1;
    int prefetch_standard = 5;
    int prefetch_high = 10;
    
    char line[80];
    
    FILE *fd;
    fd = fopen("test_line.csv", "rt");
    long int elapsed_seconds;
    while(fgets(line, 80, fd) != NULL)
   {
	 /* get a line, up to 80 chars from fd.  done if NULL */
	 sscanf (line, "%ld", &elapsed_seconds);
   }
   fclose(fd);  /* close the file prior to exiting the routine */

    //printf("Line: %s\n", line);
    //convert line of text to data
    percent = (line[0]-'0')*100 + (line[1]-'0')*10+(line[2]-'0');
    prefetch_low = (line[4]-'0')*10+(line[5]-'0');
    prefetch_standard = (line[7]-'0')*10+(line[8]-'0');
    prefetch_high = (line[10]-'0')*10+(line[11]-'0');
    //printf("Percent: %d\n", percent);
    //printf("P_low: %d\n", prefetch_low);
    //printf("P_high: %d\n", prefetch_high);
    
    int prefetch_degree_used;
    //float MPC = (float)trackers[tracker_index].miss / trackers[tracker_index].cycle_num;
    int thresh;
    thresh = (COMMON_COUNT - 1)*((float)percent/100.0);
    //printf("\n%d\n", thresh);
    //rare instruction
    if(trackers[tracker_index].cycle_num < least_common_common) {
         //if doing well
         if((float)trackers[tracker_index].miss/(float)trackers[tracker_index].cycle_num < (float)trackers[thresh].miss/(float)trackers[thresh].cycle_num)
            prefetch_degree_used = prefetch_low;
         else
            prefetch_degree_used = prefetch_standard;
    } else {
        //if doing well
        if((float)trackers[tracker_index].miss/(float)trackers[tracker_index].cycle_num < (float)trackers[thresh].miss/(float)trackers[thresh].cycle_num)
            prefetch_degree_used = prefetch_standard;
         else
            prefetch_degree_used = prefetch_high;
    }
    //printf("%f\n%d\n%d\n", thresh, tracker_index, comp);
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
      // do some prefetching
      
	
      int i, j;
      int k = 0;
		
      if(trackers[tracker_index].stream > 0) {
	      for(j=0, k=0; k<=prefetch_degree_used; j++) {

			for(i=0; i<=trackers[tracker_index].stream; i++) {
				k++;
				
				if(k>prefetch_degree_used)
				  break;
                if(i>STREAM_DEPTH)
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
				if(get_l2_mshr_occupancy(0) < 8) {
				    l2_prefetch_line(0, addr, pf_address, FILL_L2);
				}else{
				    l2_prefetch_line(0, addr, pf_address, FILL_LLC);
				  }
				
			  }
			}
	} else {
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

    } else if((addr>>6) == ((trackers[tracker_index].last_addr>>6) + 1)) {
        stride = trackers[tracker_index].last_stride; //dont want to override stride if streaming
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
        stride = trackers[tracker_index].last_stride; //dont want to override stride if streaming
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
    
  trackers[tracker_index].last_addr = addr;
  trackers[tracker_index].last_stride = stride;
}

void l2_cache_fill(int cpu_num, unsigned long long int addr, int set, int way, int prefetch, unsigned long long int evicted_addr)
{
  // uncomment this line to see the information available to you when there is a cache fill event
  //printf("0x%llx %d %d %d 0x%llx\n", addr, set, way, prefetch, evicted_addr);
}
