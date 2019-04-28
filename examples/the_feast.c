/*  Created by Gguidini and Vitor Dullens, April 16th, 2019
*
*   The feast has 1 cooker, that produces COOKER_RATE meals.
*   There's EATERS eaters, that eat continuously from a table.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define EATERS 10
#define COOKER_RATE 10

#define COOKING_TIME 1
#define EATING_TIME 1

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER;;
pthread_cond_t SLEEP_C = PTHREAD_COND_INITIALIZER;
pthread_cond_t SLEEP_E = PTHREAD_COND_INITIALIZER;

struct {
    bool table[100];
    int plates;
} table;

void add_meals(int x){ 
    pthread_mutex_lock(&LOCK);
    int i;
    for(i = 0; i < x; i++){
        if(table.plates+i == 100){
            break;
        }
        table.table[table.plates+i] = true;
    }
    table.plates += i;
    printf( BLUE "Added %d meals o table. Total: %d\n" RESET, i, table.plates);
    pthread_mutex_unlock(&LOCK);    
}

void eat_meal(){
    pthread_mutex_lock(&LOCK);
    table.plates -= 1;
    table.table[table.plates] = false;
    printf( GREEN "Meal removed. Total: %d\n" RESET, table.plates);
    pthread_mutex_unlock(&LOCK);        
}

void* cooker(void* data){
    while(true){
        printf( CYAN "Cooker is cooking\n" RESET);
        sleep(COOKING_TIME);
        pthread_mutex_lock(&LOCK);
        while(table.plates == 100){
            printf( RED "Cooker is going to sleep now\n" RESET);
            pthread_cond_wait(&SLEEP_C, &LOCK);
        }
        pthread_mutex_unlock(&LOCK);
        add_meals(COOKER_RATE);
        // Wakes up any sleeping eater
        pthread_mutex_lock(&LOCK);
        if(table.plates == COOKER_RATE){
            pthread_cond_broadcast(&SLEEP_E);
        }
        pthread_mutex_unlock(&LOCK);        
    }
}

void* eater(void* data){
    long id = (long) data;
    printf( YELLOW "Eater %ld is hungry\n" RESET, id);
    while(true) {
        pthread_mutex_lock(&LOCK);
        if(table.plates > 0){
            pthread_mutex_unlock(&LOCK);      
            printf( MAGENTA "Eater %ld is eating :D\n" RESET, id);      
            eat_meal();
            pthread_mutex_lock(&LOCK);
            if(table.plates == 0){
                pthread_cond_signal(&SLEEP_C);
            }
            pthread_mutex_unlock(&LOCK);    
        }
        pthread_mutex_unlock(&LOCK);    
        while(table.plates == 0){
            printf( RED "Eater %ld is going to sleep\n" RESET, id);
            pthread_cond_wait(&SLEEP_E, &LOCK);
        }
        pthread_mutex_unlock(&LOCK);
        sleep(EATING_TIME);        
    }
}

int main(){
    for(int i = 0; i < 100; i++){
        table.table[i] = false;
    }
    table.plates = 0;

    pthread_t cook;
    pthread_t eaters[EATERS];

    pthread_create(&cook, NULL, cooker, NULL);

    for(long i = 0; i < EATERS; i++){
        pthread_create(&eaters[i], NULL, eater, (void*) i);
    }

    pthread_join(cook, NULL);
}