// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <time.h>
// #include <string.h>
// #include <stdbool.h>

// #define RED     "\x1b[31m"
// #define GREEN   "\x1b[32m"
// #define YELLOW  "\x1b[33m"
// #define BLUE    "\x1b[34m"
// #define MAGENTA "\x1b[35m"
// #define CYAN    "\x1b[36m"
// #define RESET   "\x1b[0m"

// #define Cat_Feeding_Time = 2
// #define Dogs = 10

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

#define DOGS 5


// Locks and condition variables


pthread_mutex_t galpao = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cachorro = PTHREAD_COND_INITIALIZER;
pthread_cond_t gato = PTHREAD_COND_INITIALIZER;

int gato_quer = 0;
int gato_comendo = 0;
int caes = 0;



void * dogs(){
    while(true){
        sleep(1);
        pthread_mutex_lock(&galpao);
        if(gato_quer){
            caes--;
            printf(CYAN "Gato quer comer, cachorro saindo (%d)\n" RESET, caes);
            if(!caes){
                pthread_cond_signal(&gato);
            }
        }
        else{
            while(gato_comendo){
                pthread_cond_wait(&cachorro, &galpao);
            }
            caes++;
            printf(RED "Entrou um cachorro (%d)\n"RESET, caes);
        }
        pthread_mutex_unlock(&galpao);
    }
}

void * cat(){
    while(true){
        sleep(rand()%4 + 2);

        pthread_mutex_lock(&galpao);
        printf(YELLOW "Gato quer entrar, expulsando todos os cachorros\n" RESET);
        gato_quer = 1;
        while(caes){
            pthread_cond_wait(&gato, &galpao);
        }
        gato_quer = 0;
        gato_comendo = 1;
        pthread_mutex_unlock(&galpao);
        printf(BLUE"Gato comendo\n"RESET);
        sleep(3);
        pthread_mutex_lock(&galpao);
        printf(GREEN"Gato terminou de comer, indo embora\n" RESET);
        gato_comendo = 0;
        pthread_cond_broadcast(&cachorro);
        pthread_mutex_unlock(&galpao);
    }
}

int main(){
    pthread_t t[DOGS  + 1];
    pthread_create(&t[0], NULL, cat, NULL);

    for(int i = 1; i < DOGS + 1; i++)
        pthread_create(&t[i], NULL, dogs, NULL);


     for (int i = 0; i < DOGS + 1; i++) 
        pthread_join(t[i], NULL);

    
    pthread_exit (NULL);


}