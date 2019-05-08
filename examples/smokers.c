/* Created by Gguidini
*
*   3 men are in a room. All 3 smoke. In order to prepare a cigarette they need:
*   tobacco, paper, a match. Each man has exactly 1 of those things.
*   There's also a salesman, with an infinite supply of those things.
*   The salesman puts 2 items on sale. One of the smokers buys and uses them.
*   Once he's done, he informas the salesman, that then selects other items.
*
*
*   SOLUÇÃO:
*   O algoritmo abaixo utiliza a lógica "inversa" do vendedor para resolver o problema.
*   Isto é, ao invés de o vendedor informar quais 2 itens ele está vendendo, ele informa
*   qual item falta para completar o trio.
*   Dessa forma, cada fumante só precisa esperar pelo item que ele possui ser chamado, e saberá
*   que pode comprar as coisas que o vendedor está vendendo agora.
*/

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

#define tobacco 0
#define paper 1
#define match 2

sem_t sem_items[3], selling;

char* items[] = {"Tabaco", "Papel", "Fosforo"};

void *Seller(void* data) {
    int item_1, item_2;
    while (1) {
        // Espera até poder colocar novos itens
        sem_wait(&selling);
        printf( GREEN "Escolhendo itens para vender...\n" RESET);
        sleep(1 + rand()%5);
        item_1 = rand()%3;
        item_2 = rand()%3;
        while(item_1 == item_2){
            // Não pode vender itens repetidos
            item_2 = rand()%3;
        }
        printf(GREEN "Colocando à venda: %s e %s\n" RESET, items[item_1], items[item_2]);
        for(int i = 0; i < 3; i++){
            if(i != item_1 && i != item_2){
                // Dá o post no único item que NÃO será vendido
                sem_post(&sem_items[i]);
            }
        }
    }
}

void *Smoker(void* data){
    int has = (int) (long) data;
    while (1) {
        // Espera post no item que eu já tenho.
        sem_wait(&sem_items[has]);
        printf(YELLOW "Fumante com %s: Comprei os items\n" RESET, items[has]);
        sleep(1 + rand()%5);
        printf(RED "Terminei de fumar coff cof\n" RESET);
        sem_post(&selling);
    }
    
}

int main() {
    sem_init(&selling, 0, 1);
    for(int i = 0; i < 3; i++){
        sem_init(&sem_items[i], 0, 0);
    }
    pthread_t salesman, smokers[3];

    pthread_create(&salesman, NULL, Seller, NULL);
    for(long i = 0; i < 3; i++){
        pthread_create(&smokers[i], NULL, Smoker, (void*) (i));
    }

    pthread_join(salesman, NULL);
}