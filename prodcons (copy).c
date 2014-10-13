/* 
 * Operating Systems  (2IN05)  Practical Assignment
 * Condition Variables Application
 *
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include "prodcons.h"

/* buffer[]
 * circular buffer that holds the items generated by the producer and
 * which have to be retrieved by consumers
 */
static ITEM   buffer [BUFFER_SIZE];

static void rsleep (int t);

void signal_test_handler(int signum);

/* producer thread */
static void * producer (void * arg)
{
    ITEM    item;   // a produced item
    
    while (1/* TODO: not all items produced */)
    {
        rsleep (PRODUCER_SLEEP_FACTOR);
        
        // TODO: produce new item and put it into buffer[]
        
        printf("%04x\n", item); // write info to stdout
    }
    
    // TODO: inform consumers that we're ready
}

/* consumer thread */
static void * consumer (void * arg)
{
    ITEM    item;   // a consumed item
    int     id;     // identifier of this consumer (value 0..NROF_CONSUMERS-1)
    
    while (1/* TODO: not all items retrieved for this customer */)
    {
        rsleep (100 * NROF_CONSUMERS);

        // TODO: get the next item from buffer[] (intended for this customer)

        printf("%*s    C%d:%04x\n", 7*id, "", id, item); // write info to stdout (with indentation)
    }
}

int main (void)
{
	printf("Producer / consumer 0.0.0\n");

	signal(SIGINT, signal_test_handler);

	while(true)
	{
		printf("Processing...\n");
		sleep(1);
	}

    // TODO: startup the producer thread and the consumer threads
    
    // TODO: wait until all threads are finished     
    
    return (0);
}


void signal_test_handler(int signum)
{
	printf("Caught signal %d\n", signum);

	exit(signum);
}

/*
 * rsleep(int t)
 *
 * The calling thread will be suspended for a random amount of time between 0 and t microseconds
 * At the first call, the random generator is seeded with the current time
 */
static void rsleep (int t)
{
    static bool first_call = true;
    
    if (first_call == true)
    {
        srandom (time(NULL));
        first_call = false;
    }
    usleep (random () % t);
}

