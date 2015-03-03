######Skeleton:

This does absolutely nothing. Provides a simple framework off of which we can build.

######Next Line:

This simply fetches the next line whenever an adress is requested. 

######Stream:

The stream prefetcher works off of spatial locality by establishing a "stream" in which the program first checks if the accessed page is one of the 64 pages being kept track of. If it isn't, the oldest page is overwritten with this new page. It then checks to see if the page offset has increased or decreased. If it has increased, it checks to see which direction was already stored. If the direction of the page offset and the direction stored are the same, the confidence increases, otherwise the confidence is reset to 0 and the direction stored is flipped.

If the confidence is high enough (2 or greater), then the next two lines are prefetched into either the L2 cache or the LLC depending on occupancy.

######Stride:

The instruction pointer based stride prefetcher keeps track of which adressess a specific instruction accessed most recently. It then assumes that future addresses to be accessed will be some fixed stride length away. The inner workings are similar to the stream, but this prefetcher stores and checks instruction pointers rather than pages. If it has seen a specific stride at between the last 3 memory addresses accessed, it prefetches the next 3 adresses that are that stride away.

######Access Map Pattern Matching:

