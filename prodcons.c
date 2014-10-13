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

#include "prodcons.h"

/* buffer[]
 * circular buffer that holds the items generated by the producer and
 * which have to be retrieved by consumers
 */
static ITEM   buffer [BUFFER_SIZE];

static bool   bitmap [BUFFER_SIZE];

static unsigned int nrof_consumers = NROF_CONSUMERS;

static bool done = false;

#define ONEZ 65535
#define NROF_BITS_SEQ 16-NROF_BITS_DEST

struct ThreadData
{
    int id;
};

struct bin_sem {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
};

void binsem_init(struct bin_sem *p, int init_val)
{
    pthread_mutex_lock(&p->mutex);
    if (init_val > 0)
        p->v = 1;
    else if (init_val < 1)
        p->v = 0;
    pthread_mutex_unlock(&p->mutex);
}

void binsem_post(struct bin_sem *p)
{
    pthread_mutex_lock(&p->mutex);
    if (p->v == 0)
        p->v += 1;
    pthread_cond_signal(&p->cond);
    pthread_mutex_unlock(&p->mutex);
}

void binsem_wait(struct bin_sem *p)
{
    pthread_mutex_lock(&p->mutex);
    while (!p->v)
        pthread_cond_wait(&p->cond, &p->mutex);
    p->v -= 1;
    pthread_mutex_unlock(&p->mutex);
}

pthread_cond_t cond_var[NROF_CONSUMERS];
pthread_mutex_t mutex_prod;
sem_t sem;

struct bin_sem binsem_cons[NROF_CONSUMERS];

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
    // allocate memory for thread data
    for( i = 0 ; i < nrof_consumers ; i++ )
    {
        threadData[i] = calloc(1, sizeof(struct ThreadData)); 
        threadData[i]->id = i;
        binsem_init(&binsem_cons[i], 0);
    }

    for ( i = 0; i < BUFFER_SIZE; i++ )
    {
        bitmap[i] = false;
    }

    if ( (err = sem_init(&sem, false, BUFFER_SIZE)) != 0 )
    {
        fprintf(stderr, "Error initializing semaphore %lx. %d\n", sem, err);
    }

/////////////////////////////////////////////////
// create threads
    if ( (err = pthread_mutex_lock(&mutex_prod)) != 0 )
    {
        fprintf(stderr, "Error locking mutex in main. %d\n", err);
    }
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
    if ( (err = pthread_mutex_unlock(&mutex_prod)) != 0 )
    {
        fprintf(stderr, "Error unlocking mutex in main. %d\n", err);
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
    unsigned int count = 1, i, last = NROF_ITEMS + 1, ind, err;
    
    while (1/* TODO: not all items produced */)
    {
        rsleep (PRODUCER_SLEEP_FACTOR);
        
        item = random () % nrof_consumers;
        item &= ONEZ >> NROF_BITS_SEQ;
        item |= count << NROF_BITS_DEST;
        
        if ( (err = sem_wait(&sem)) != 0)
        {
            fprintf(stderr, "Error in sem_wait() in producer. %d\n", err);
        }
        if ( (err = pthread_mutex_lock(&mutex_prod)) != 0 )
        {
            fprintf(stderr, "Error locking mutex in producer. %d\n", err);
        }

/////////////////////////////////////////////////////
// critical section

        buffer[( item >> NROF_BITS_DEST ) % BUFFER_SIZE] = item;
        bitmap[( item >> NROF_BITS_DEST ) % BUFFER_SIZE] = true;
        printf("%04x\n", item); 

        last = NROF_ITEMS + 1;
        for ( i = 0; i < BUFFER_SIZE; i++ )
        {
            if ( (buffer[i] >> NROF_BITS_DEST) < last && bitmap[i] )
            {
                last = (buffer[i] >> NROF_BITS_DEST);
                ind = i;
            }
        }

        binsem_post(&binsem_cons[buffer[ind] & (ONEZ >> NROF_BITS_SEQ)]);
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// critical section

        if ( (err = pthread_mutex_unlock(&mutex_prod)) != 0 )
        {
            fprintf(stderr, "Error unlocking mutex in producer. %d\n", err);
        }
        
        if ( count++ == NROF_ITEMS )
        {
            break;
        }
    }
    pthread_exit(NULL);
    fprintf(stderr, "Error in producer, pthread_exit() returned.\n");
}

/* consumer thread 
 * A binary semaphore (binsem_cons[]) which is constructed out of a mutex and a condition variable
 * makes sure that the consumer thread only run wen they can consume an item.
 * The mutex_prod makes sure that producer and consumer do not access the buffer simultaneously.
 */
static void * consumer (void * arg)
{
    ITEM    item;   // a consstatic void * producer (void * arg)umed item
    int     id = ((struct ThreadData *) arg)->id;
    int     i, ind, err;
    unsigned short int last = NROF_ITEMS + 1;

    
    while (1/* TODO: not all items retrieved for this customer */)
    {
        
        rsleep (100 * nrof_consumers);
        binsem_wait(&binsem_cons[id]);
        if (done)
            break;
        if ( (err = pthread_mutex_lock(&mutex_prod)) != 0 )
        {
            fprintf(stderr, "Error locking mutex in C%d. %d\n", id, err);
        }

/////////////////////////////////////////////////////
// critical section
        last = NROF_ITEMS + 1;
        for ( i = 0; i < BUFFER_SIZE; i++ )
        {
            if ( (buffer[i] >> NROF_BITS_DEST) < last && bitmap[i] )
            {
                last = buffer[i] >> NROF_BITS_DEST;
                ind = i;
            }
        }
        
        while ( (buffer[ind] & (ONEZ >> NROF_BITS_SEQ)) == id && bitmap[ind] )
        {
            item = buffer[ind];
            bitmap[ind] = false;
            ind = (ind + 1) % BUFFER_SIZE;
            printf("%*s    C%d:%04x\n", 7*id, "", id, item); // write info to stdout (with indentation)
            if ( (err = sem_post(&sem)) != 0)
            {
                fprintf(stderr, "Error in sem_post() in C%d. %d\n", id, err);
            }
        }
        if ( (buffer[ind] & (ONEZ >> (NROF_BITS_SEQ))) != id && bitmap[ind] )
        {
            binsem_post(&binsem_cons[(buffer[ind] & (ONEZ >> (NROF_BITS_SEQ) ))]);
        }
        

        if ( (item >> NROF_BITS_DEST) >= NROF_ITEMS )
        {
            done = true;
            for ( i = 0; i < nrof_consumers; i++)
            {
                binsem_post(&binsem_cons[i]);
            }
            
        }
//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
// critical section
        if ( (err = pthread_mutex_unlock(&mutex_prod)) != 0 )
        {
            fprintf(stderr, "Error unlocking mutex in C%d. %d\n", id, err);
        }

        if (done)
            break;
    }
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

