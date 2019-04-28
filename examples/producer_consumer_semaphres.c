#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <semaphore.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
sem_t empty, filled;

struct {
    int items[100];
    int idx_c;
    int idx_p;
} buffer;

int rm_item(){
    pthread_mutex_lock(&lock);
    int item = buffer.items[buffer.idx_c];
    buffer.idx_c = (buffer.idx_c + 1)%100;
    pthread_mutex_unlock(&lock);
    return item;
}

void add_item(int item){
    pthread_mutex_lock(&lock);
    buffer.items[buffer.idx_p] = item;
    buffer.idx_p = (buffer.idx_p + 1)%100;
    pthread_mutex_unlock(&lock);
}

void* consumer(void* data){
    long id = (long) data;
    int item;
    while(true) {
        sem_wait(&filled);
        item = rm_item();
        printf(GREEN "Consumer %ld removed item %d from buffer\n" RESET, id, item);
        sem_post(&empty);

        sleep(rand()%5);
    }
}

void* producer(void* data){
    long id = (long) data;
    int item;
    while(true) {
        item = rand()%1000;
        sem_wait(&empty);
        printf(YELLOW "Producer %ld adding item %d to buffer\n" RESET, id, item);
        add_item(item);
        sem_post(&filled);

        sleep(rand()%5);
    }
}

int main(){
    pthread_t Consumers[10], Producers[10];

    sem_init(&empty, 0, 100);
    sem_init(&filled, 0, 0);

    for(long i = 0; i < 10; i++){
        pthread_create(&Consumers[i], NULL, consumer, (void*) i);
        pthread_create(&Producers[i], NULL, producer, (void*) i);
    }

    pthread_join(Consumers[0], NULL);
}