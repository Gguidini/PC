// Sleepy barber problem
// Giovanni M Guidini

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define PLACES 10
#define CUSTOMER 15

sem_t wake_barber, cut_hair, wait_turn, barber_chair;

void* Barber(void* data) {
    printf("Abrindo a barbearia.\n");
    while (1){
        sem_wait(&wake_barber);
        printf("Cortando cabelo...");
        sleep(rand()%5 + 0.5);
        printf("pronto!\n");
        sem_post(&cut_hair);
    }
}

void* Customer(void* data){
    long id = (long) data;
    while(1) {
        int r = sem_trywait(&wait_turn);
        if(r != 0){
            // Can't grab a spot
            printf("%ld: Decidi virar  hippie :D\n", id);
            sleep(rand()%10 + 1);
        } else {
            printf("%ld: Aguardando para cortar o cabelo :|\n", id);
            sem_wait(&barber_chair);
            printf("%ld: Vou cortar o cabelo agora :'(\n", id);
            sem_post(&wait_turn);       // Free chair I was using
            sem_post(&wake_barber);     // Wake the barber to cut my hair
            sem_wait(&cut_hair);        // Waits for barber to finish my hair
            printf("%ld: Saindo da barbearia TT-TT\n", id);
            sem_post(&barber_chair);    // Free barber chair
        }
        sleep(rand()%10);
    }
}

int main() {
    sem_init(&wake_barber, 0, 0);
    sem_init(&barber_chair, 0, 1);
    sem_init(&cut_hair, 0, 0);
    sem_init(&wait_turn, 0, PLACES);

    pthread_t barber, customers[CUSTOMER];
    pthread_create(&barber, NULL, Barber, NULL);

    for(long i = 0; i < CUSTOMER; i++){
        pthread_create(&customers[i], NULL, Customer, (void*) i);
    }

    pthread_join(barber, NULL);
}