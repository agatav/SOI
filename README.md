# SOI

# folder Semaphore 
The solution to the producers&consumers problem. There is FIFO buffer of 9 elements. The producer A puts 1 element into buffer,
the producer B puts 3 ones. The consumer A gets 1 element, B one gets 2 elements from the buffer. Size of the buffer can vary
from 3 to 9, at the start buffer is empty.

#Monitor.h, monitor.cpp
There are used monitors to solve above problem.
