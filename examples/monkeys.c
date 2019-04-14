/*  Created by Gguidini, April 9, 2019
*
*   This program refeers to the Monkey's problem. It is as follows:
*   There's a bridge in the middle of the forest, and some monkeys on either side of it.
*   The monkeys eventually cross the bridge for food, and go back.   
*   The bridge can take infinite monkeys, as long as all the monkeys are going to the same side.
*
*/

#define MONKEYS 10 // number of monkeys on either side of the bridge

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

pthread_mutex_t BRIDGE = PTHREAD_MUTEX_INITIALIZER; // bridge lock
pthread_mutex_t LEFT = PTHREAD_MUTEX_INITIALIZER; // left side lock
pthread_mutex_t RIGHT = PTHREAD_MUTEX_INITIALIZER; // right side lock
pthread_mutex_t TURN = PTHREAD_MUTEX_INITIALIZER; // taking turns lock

int left = 0;       // number of monkeys on the left side wanting to go right
int right = 0;      // number of monkeys on the right side wanting to go left

// Every monkey is in a side of the bridge.
// They change sides nonestop.
// crossing the bridge takes 0.8s
void* monkey(void* info){
    sleep(1);   // wait all monkeys to be created
    long side = (long) info;    // initial side
    while(1) {
        if(side) {  // on the right side, going to the left side
            pthread_mutex_lock(&TURN);
            pthread_mutex_lock(&RIGHT);
            right++;    // intention of going to the left side
            if (right == 1){
                pthread_mutex_lock(&BRIDGE);
            }
            printf( GREEN "Macacos indo para a esquerda - %d\n" RESET, right);
            pthread_mutex_unlock(&RIGHT);
            pthread_mutex_unlock(&TURN);            
            side = side == 1 ? 0 : 1;   // change sides
            sleep(0.8);
            pthread_mutex_lock(&RIGHT);
            right--;    // finished crossing to the left side
            printf( BLUE "Macaco chegou na esquerda\n" RESET);
            if (right == 0){
                pthread_mutex_unlock(&BRIDGE);
            }
            pthread_mutex_unlock(&RIGHT);
        }
        else {  // on the left side, going to the right side
            pthread_mutex_lock(&TURN);
            pthread_mutex_lock(&LEFT);
            left++;
            if (left == 1){
                pthread_mutex_lock(&BRIDGE);
            }
            printf( RED "Macacos indo para a direita - %d\n" RESET, left);
            pthread_mutex_unlock(&LEFT);
            pthread_mutex_unlock(&TURN);
            side = side == 1 ? 0 : 1;   // change sides
            sleep(0.8);
            pthread_mutex_lock(&LEFT);
            left--;
            printf( MAGENTA "Macaco chegou na direita\n" RESET);
            if (left == 0){
                pthread_mutex_unlock(&BRIDGE);
            }
            pthread_mutex_unlock(&LEFT);
        }
    }
}

int main() {
    pthread_t monkeys[MONKEYS * 2];
    for(int i = 0; i < MONKEYS; i++) {
        pthread_create(&monkeys[i], NULL, monkey, (void*) 0);
        pthread_create(&monkeys[i + MONKEYS], NULL, monkey, (void*) 1);
    }

    // monkey threads are eternal
    pthread_join(monkeys[0], NULL);
}