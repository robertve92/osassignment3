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
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include "prodcons.h"

/* buffer[]
 * circular buffer that holds the items generated by the producer and
 * which have to be retrieved by consumers
 */
static ITEM   buffer [BUFFER_SIZE];
static unsigned int nrof_consumers = NROF_CONSUMERS;
static int consumedItems = 0;

#define ONEZ 65535
#define NROF_BITS_SEQ 16-NROF_BITS_DEST

struct ThreadData
{
    int id;
};

int numerItemsInBuffer;
pthread_cond_t consumerVariables[NROF_CONSUMERS];
pthread_cond_t producerVariable;
pthread_mutex_t sharedMutex;

int itemToSequenceNR(ITEM item){
    //printf("seqNr: %d -> %d\n", item, item >> NROF_BITS_DEST);
    return item >> NROF_BITS_DEST;
}

int itemToDest(ITEM item){
    //printf("dest: %d -> %d\n", item, (item & (ONEZ >> NROF_BITS_SEQ)));
    return (item & (ONEZ >> NROF_BITS_SEQ));
}

void placeItem(ITEM item){
    int i = 0;
    // We start by assering that the buffer is not yet full
    assert(buffer[BUFFER_SIZE - 1] == NULL);
    assert(numerItemsInBuffer < BUFFER_SIZE);
    // We find the first free place in the buffer 
    while(buffer[i] != NULL){
        i += 1;        
    }
    // And place the item there
    buffer[i] = item;
    // we increment the nr of items in the buffer
    numerItemsInBuffer += 1;
    // And finally we signal the consumer that has to come into action next
    pthread_cond_signal(&consumerVariables[itemToDest(item)]);
}

ITEM takeItem(){
    int i = 0;
    // We start by asserting that the bufer is not empty
    assert(buffer[0] != NULL);
    // We store the return value (the first value in the buffer)
    int rv = buffer[0];   
    // We move all items one place to the front
    for(i = 0 ; i < BUFFER_SIZE - 1 ; i += 1){
        buffer[i] = buffer[i + 1]; 
    }
    // We set the last place in the buffer to NULL, indicating that it is empty now
    buffer[BUFFER_SIZE - 1] = NULL;
    // We indicate that there is one less item in the buffer now
    numerItemsInBuffer -= 1;
    // We signal the producer
    pthread_cond_signal(&producerVariable);
    // If there are items in the buffer
    if(numerItemsInBuffer > 0){
        // We also signal the next consumer
        // Note that we are at any point only looking at the first remaining item in the buffer!
        pthread_cond_signal(&consumerVariables[itemToDest(buffer[0])]);
    }
    // And return the return value
    return rv;
};

static void * producer (void * arg);
static void * consumer (void * arg);
static void rsleep (int t);

int main (void)
{
    if (NROF_ITEMS <= 0)
        return(0);

	pthread_t p_producer;
	pthread_t p_consumer[nrof_consumers];
	int i, err = 0;

    struct ThreadData * threadData[nrof_consumers];
    // allocate memory for thread data and initialise the condition variable
    for( i = 0 ; i < nrof_consumers ; i++ ){
        threadData[i] = calloc(1, sizeof(struct ThreadData)); 
        threadData[i]->id = i;
        pthread_cond_init(&consumerVariables[i], NULL);
    }
    // empty the buffer
    // note that because the first item is labeled '1', an actual item will never equal NULL
    for( i = 0 ; i < BUFFER_SIZE ; i += 1){
        buffer[i] = NULL;
    }
    //initialise the producer's condition variable
    pthread_cond_init(&producerVariable, NULL);
    // initialise the mutex
    pthread_mutex_init(&sharedMutex, NULL);
    // Indicate that there are 0 items in the buffer
    numerItemsInBuffer = 0;
    
/////////////////////////////////////////////////
// create threads
    if ( (err = pthread_create(&p_producer, NULL, producer, NULL)) != 0 )
    {
        fprintf(stderr, "Error creating producer thread %lx. %d\n", p_producer, err);
    }
	for (i = 0; i < nrof_consumers; i++)
	{
        if ( (err = pthread_create(&p_consumer[i], NULL, consumer, threadData[i])) != 0 )
        {
            fprintf(stderr, "Error creating consumer thread %lx. %d\n", p_consumer[i], err);
        }
	}
/////////////////////////////////////////////////
// clean up threads
	if ( (err = pthread_join (p_producer, NULL ) ) != 0 )
	{
		fprintf(stderr, "Error joining producer thread %lx. %d\n", p_producer, err);
	}
	for (i = 0; i < nrof_consumers; i++)
	{
		if ( ( err = pthread_join(p_consumer[i], NULL ) != 0 ) )
		{
			fprintf(stderr, "Error joining consumer thread %lx. %d\n", p_consumer[i], err);
		}
	}  
    
    return (0);
}


/* producer thread */
static void * producer (void * arg)
{
    ITEM    item;   // a produced item
    unsigned int count = 1, i, ind, err;
    
    while (1){
        rsleep (PRODUCER_SLEEP_FACTOR);
        //First we determine what consumer this item is for        
        item = random () % nrof_consumers;
        // Do some shifting and and-ing to place that information in the appropriate position
        item &= ONEZ >> NROF_BITS_SEQ;
        // And add the actually item 'message'; the count
        item |= count << NROF_BITS_DEST;
        //We for there to be a free space in the buffer
        pthread_mutex_lock(&sharedMutex);
        while(numerItemsInBuffer >= BUFFER_SIZE){
            pthread_cond_wait(&producerVariable, &sharedMutex);
        }
        //We use the lock on the shared mutex that we still own at this point
/////////////////////////////////////////////////////
// critical section
        // We place an item into the buffer
        // This also signals the consumer that has to retrieve the first item in the buffer
        placeItem(item);
        printf("%04x\n", item); 
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// critical section
        pthread_mutex_unlock(&sharedMutex);
        //printf("prod released mutex\n");
        if ( count++ == NROF_ITEMS ){
            break;
        }
    }
    fprintf(stderr, "prod term\n");
    pthread_exit(NULL);
    fprintf(stderr, "Error in producer, pthread_exit() returned.\n");
}

static void * consumer (void * arg)
{
    ITEM    item;   // a consstatic void * producer (void * arg)umed item
    int     id = ((struct ThreadData *) arg)->id;
    int     i, ind, err;
    unsigned short int last = NROF_ITEMS + 1;

    
    while (1){
        rsleep (100 * nrof_consumers);
        //We for there to be an item in the buffer
        pthread_mutex_lock(&sharedMutex);
        // While the buffer is empty, or the first item in the buffer is not ours, and we are not done
        while(consumedItems != NROF_ITEMS && (numerItemsInBuffer == 0 || itemToDest(buffer[0]) != id)){
            // We wait
            pthread_cond_wait(&consumerVariables[id], &sharedMutex);
        }
        // If we are done
        if(consumedItems == NROF_ITEMS){
            //We quit
            pthread_mutex_unlock(&sharedMutex);
            break;
        }
        //We use the lock on the shared mutex that we still own at this point
/////////////////////////////////////////////////////
// critical section
        // We take an item out of the buffer
        // This also signals the producer and the next consumer (if needed)
        item = takeItem(id);
        printf("%*s    C%d:%04x\n", 7*id, "", id, item); // write info to stdout (with indentation) 
        //We keep track of the number of consumed items
        consumedItems += 1;
        //If we are done
        if(consumedItems == NROF_ITEMS){
            //We signal all consumer that might currently be waiting for more items to be produced
            for(i = 0 ; i < NROF_CONSUMERS ; i += 1){
                if(i != id)
                    pthread_cond_signal(&consumerVariables[i]);
            }
            //And we quit ourselves
            pthread_mutex_unlock(&sharedMutex);
            break; 
        }
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// critical section
        pthread_mutex_unlock(&sharedMutex);
        
    }
    fprintf(stderr, "cons %d term!\n", id);
    pthread_exit(NULL);
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

