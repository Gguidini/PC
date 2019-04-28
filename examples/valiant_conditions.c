// Giovanni M Guidini. 16/0122660
// 
// Um pombo correio leva cartas entre os sites A e B, mas só quando o número de cartas acumuladas em sua mochila chegar a 20. Inicialmente, o pombo fica em A, esperando que existam 20 cartas para carregar, e dormindo enquanto não houver. Quando as cartas chegam a 20, o pombo deve levar todas as 20 cartas de A para B, e em seguida voltar para A, repetindo este ciclo. As cartas são escritas por uma quantidade qualquer de usuários. Cada usuário, quando tem uma carta pronta, coloca sua carta na mochila do pombo e volta a escrever uma nova carta. Caso o pombo tenha partido, ele deve esperar o seu retorno para colar a carta na mochila.

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

#define BK_RED "\x1b[41m"
#define WRITERS 10

pthread_cond_t deliver = PTHREAD_COND_INITIALIZER;
pthread_cond_t wait_valiant_return = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock_letters = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_valiant = PTHREAD_MUTEX_INITIALIZER;

int letters_in_bag;
bool is_valiant_here;

void* Valiant() {
    printf(BK_RED "Valiant is ready to deliver!\n" RESET);
    while(1){
        pthread_mutex_lock(&lock_letters);
            while(letters_in_bag != 20){
                pthread_cond_wait(&deliver, &lock_letters);
            }
        pthread_mutex_lock(&lock_valiant);
        is_valiant_here = false;
        letters_in_bag = 0;
        pthread_mutex_unlock(&lock_valiant);
        pthread_mutex_unlock(&lock_letters);
        printf(RED "Valiant is going behind enemy lines\n");
        sleep(2 + rand()%5);
        pthread_mutex_lock(&lock_valiant);
        printf(RED "Valiant has returned!\n");
        is_valiant_here = true;
        pthread_cond_broadcast(&wait_valiant_return);
        pthread_mutex_unlock(&lock_valiant);
    }
}

void* Writer(void* data) {
    long id = (long) data;
    while(1){
        pthread_mutex_lock(&lock_valiant);
        if(!is_valiant_here){
            printf(YELLOW "%ld: Valiant isn't here. I'll wait.\n", id);
            pthread_cond_wait(&wait_valiant_return, &lock_valiant);
        }
        pthread_mutex_lock(&lock_letters);
        letters_in_bag++;
        printf(CYAN "%ld: Put my letter in bag. Now there are %d letters.\n", id, letters_in_bag);
        if(letters_in_bag == 20){
            printf(GREEN "%ld: Waking up Valiant because bag is full\n", id);
            pthread_cond_signal(&deliver);
        }
        pthread_mutex_unlock(&lock_letters);
        pthread_mutex_unlock(&lock_valiant);
        sleep(rand()%15 + 1);
    }
}

int main() {
    pthread_t pigeon, writers[WRITERS];

    letters_in_bag = 0;
    is_valiant_here = true;

    pthread_create(&pigeon, NULL, Valiant, NULL);
    for(long i = 0; i < WRITERS; i++) {
        pthread_create(&writers[i], NULL, Writer, (void*) i);
    }

    pthread_join(pigeon, NULL);
}

