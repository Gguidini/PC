// Giovanni M Guidini. 16/0122660
// 
// Um pombo correio leva cartas entre os sites A e B, mas só quando o número de cartas acumuladas em sua mochila chegar a 20. Inicialmente, o pombo fica em A, esperando que existam 20 cartas para carregar, e dormindo enquanto não houver. Quando as cartas chegam a 20, o pombo deve levar todas as 20 cartas de A para B, e em seguida voltar para A, repetindo este ciclo. As cartas são escritas por uma quantidade qualquer de usuários. Cada usuário, quando tem uma carta pronta, coloca sua carta na mochila do pombo e volta a escrever uma nova carta. Caso o pombo tenha partido, ele deve esperar o seu retorno para colar a carta na mochila.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
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

sem_t deliver, returned, bag;

int letters_in_bag;
bool is_valiant_here;

void* Valiant() {
    printf(BK_RED "Valiant is ready to deliver!\n" RESET);
    while(1){
       
        printf(RED "Valiant is going behind enemy lines\n");
        
        printf(RED "Valiant has returned!\n");
       
    }
}

void* Writer(void* data) {
    long id = (long) data;
    while(1){
        
        printf(YELLOW "%ld: Valiant isn't here. I'll wait.\n", id);
        
        printf(CYAN "%ld: Put my letter in bag. Now there are %d letters.\n", id, letters_in_bag);
        
        printf(GREEN "%ld: Waking up Valiant because bag is full\n", id);
        
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

