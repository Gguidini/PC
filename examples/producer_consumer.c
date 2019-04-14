/*  Created by Gguidini, April 13, 2019
*
*   The producers and consumers problem can be stated as follows:
*   P Producers continouslly produce some data, that is stored in a buffer,
*   C consumers consume the producted data from the buffer.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

#define PRODUCERS 4
#define CONSUMERS 4

// Define who's faster (consumer or producers)
// (0) - Same speed for both
// (1) - Consumers are faster
// (2) - Producers are faster
#define FAST 0

#if FAST == 0
    #define CONSUMER_WAIT 1
    #define PRODUCER_WAIT 1
#elif FAST == 1
    #define CONSUMER_WAIT 1
    #define PRODUCER_WAIT 2
#else
    #define CONSUMER_WAIT 2
    #define PRODUCER_WAIT 1
#endif

// The buffer struct. Holds the buffer and pointers.
struct {
    int buff[100];
    int idx_p;
    int idx_c;
} buffer;

// Item's counter. Buffer size.
int cnt = 0;
int BUFFER_SIZE = 100;

// Locks and condition variables
pthread_mutex_t BUFFER = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CNT = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t SLEEP_C = PTHREAD_COND_INITIALIZER;
pthread_cond_t SLEEP_P = PTHREAD_COND_INITIALIZER;

// add item to buffer.
// Guaranteed that there's space for the item.
void add_item(int item){
    pthread_mutex_lock(&BUFFER);
    buffer.buff[buffer.idx_p] = item;
    buffer.idx_p = (buffer.idx_p + 1)%BUFFER_SIZE;
    pthread_mutex_unlock(&BUFFER);
}

// read item from buffer.
// Guaranteed that there's an item to be read.
int read_item(){
    int item;
    pthread_mutex_lock(&BUFFER);
    item = buffer.buff[buffer.idx_c];
    buffer.idx_c = (buffer.idx_c + 1)%BUFFER_SIZE;
    pthread_mutex_unlock(&BUFFER);
    return item;
}

// The producers thread
void* producer(void* data){
    int item;
    long id = (long) data;
    while(true){
        // create item
        item = rand()%1000;
        pthread_mutex_lock(&CNT);
        // verify that there's space for my item
        while (cnt == BUFFER_SIZE){
            printf( YELLOW "Producer %ld going to sleep\n" RESET, id);
            pthread_cond_wait(&SLEEP_P, &CNT);
        }
        cnt++;
        pthread_mutex_unlock(&CNT);
        printf( GREEN "Producer %ld is adding %d to buffer\n" RESET, id, item);
        add_item(item);
        pthread_mutex_lock(&CNT);
        // Verify if needs to wake up consumers
        if(cnt <= CONSUMERS){
            pthread_cond_signal(&SLEEP_C);
        }
        pthread_mutex_unlock(&CNT);

        // Fakes next item's production
        sleep(PRODUCER_WAIT);
    }
}

void* consumer(void* data){
    int item;
    long id = (long) data;
    while(true){
        pthread_mutex_lock(&CNT);
        // verify that there's space for my item
        while (cnt == 0){
            printf( RED "Consumer %ld going to sleep\n" RESET, id);
            pthread_cond_wait(&SLEEP_C, &CNT);
        }
        cnt--;
        pthread_mutex_unlock(&CNT);
        item = read_item();
        printf( BLUE "Consumer %ld read %d from buffer\n" RESET, id, item);
        pthread_mutex_lock(&CNT);
        // Verify if needs to wake up consumers
        if(cnt >= BUFFER_SIZE - PRODUCERS){
            pthread_cond_signal(&SLEEP_P);
        }
        pthread_mutex_unlock(&CNT);

        // Fakes next item's production
        sleep(CONSUMER_WAIT);
    }
}

int main(){
    // init buffer
    buffer.idx_c = buffer.idx_p = 0;
    // create threads
    pthread_t consumers[CONSUMERS];
    pthread_t producers[PRODUCERS];

    for(long i = 0; i < CONSUMERS; i++){
        pthread_create(&consumers[i], NULL, consumer, (void*) i);
    }
    for(long i = 0; i < PRODUCERS; i++){
        pthread_create(&producers[i], NULL, producer, (void*) i);
    }

    // prevent main from exiting
    pthread_join(consumers[0], NULL);
}


