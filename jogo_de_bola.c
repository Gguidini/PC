/* Criado por Giovanni M Guidini - 16/0122660
    Em um condomínio existem N crianças. M (M < N) dessas crianças possuem uma bola.

    As crianças brincam de bola na quadra do condomínio. Crianças só podem brincar se
    alguma entre elas possuem a bola, e somente se existir um número PAR de crianças na
    quadra (se houver um número IMPAR a última criança precisa esperar alguém sair, ou
    que mais alguém chegue).

    Eventualmente as mães das crianças as chamam para casa, e nesse momento ela deve parar
    de brincar. Se a criança chamada é a única dona de uma bola, a brincadeira acaba.
    
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define KIDS 24
#define BALL 6

#define LETS_PLAY 0
#define COME_HOME 1
#define WANNA_PLAY 3

pthread_mutex_t court = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t states; // Lock recursivo
pthread_mutexattr_t states_attr;
pthread_cond_t lets_play = PTHREAD_COND_INITIALIZER;

sem_t child_pb[KIDS];   // Semaforo da criança
int child_state[KIDS];  // Situação da criança na quadra
bool played[KIDS];      // Se a criança ja brincou ou nao

int kids_in_court;
int ball_owners_in_court;
int kids_waiting_to_play;

// child_t guarda as informações da criança.
// Uma criança tem um ID, nome
// E ela pode ou não ter uma bola.
typedef struct {
    int id;
    char* name;
    bool has_ball;
} child_t;

// get_my_state retorna o estado atual de uma criança
// Função atômica.
int get_my_state(int id){
    pthread_mutex_lock(&states);
    int s = child_state[id];
    pthread_mutex_unlock(&states);
    return s;
}

// set_my_state atualiza o estado de uma criança
// Função atômica
void set_my_state(int id, int state){
    pthread_mutex_lock(&states);
    child_state[id] = state;
    pthread_mutex_unlock(&states);
}

void* child(void* data){
    // Info da criança
    child_t* info = (child_t*) data;
    sleep(1);
    while(true) {
        // Criança quer brincar
        pthread_mutex_lock(&court);
        if(info->has_ball){
            // Criança com bola ja vai brincar
            // E chama os amiguinhos para brincar tambem
            ball_owners_in_court++;
            pthread_cond_broadcast(&lets_play);
        } else {
            // Nenhuma crinaça esta brincando
            // Espera alguem com bola chamar para brincar
            while(ball_owners_in_court == 0){
                pthread_cond_wait(&lets_play, &court);
            }
        }
        kids_in_court++;
        played[info->id] = false;
        // printf("%s (%d): Entrou na quadra. %d pessoas na quadra\n", info->name, info->id, kids_in_court);
        set_my_state(info->id, WANNA_PLAY);
        pthread_mutex_unlock(&court);
        // Criança esta na quadra
        // Precisa aguardar mensagens:
        // Se o numero de criancas eh par, para poder jogar
        // Se a mae esta chamando de volta
        // Se o dono da bola foi embora
        int s = get_my_state(info->id);
        while(s != COME_HOME){
            // printf("%s (%d): Meu estado é %d\n", info->name, info->id, s);
            pthread_mutex_lock(&states); 
            if(s == WANNA_PLAY && kids_waiting_to_play != -1){
                // Criança quer brincar e existe um numero impar de crianças que tambem querem
                // Libera a outra crinaça que está esperando
                // printf("%s (%d): Quero brincar e (%d) estava esperando para brincar\n",
                // info->name, info->id, kids_waiting_to_play);
                set_my_state(info->id, LETS_PLAY);
                set_my_state(kids_waiting_to_play, LETS_PLAY);
                played[kids_waiting_to_play] = true;
                played[info->id] = true;
                kids_waiting_to_play = -1;
            } else if(s == WANNA_PLAY && kids_waiting_to_play == -1){
                // printf("%s (%d): Quero brincar, mas tenho que esperar mais alguem\n", info->name, info->id);
                kids_waiting_to_play = info->id;
                played[info->id] = false;                
            }
            pthread_mutex_unlock(&states); 
            // Espera alguma ação que precisa de resposta
            sem_wait(&child_pb[info->id]);
            s = get_my_state(info->id);     
            // printf("%s (%d): Fui acordada com estado %d\n", info->name, info->id, s);  
        }
        // Clean up antes de sair
        pthread_mutex_lock(&states);
        bool local_played = played[info->id];
        // printf("%s (%d): Saindo da quadra. [Played %d] [Kids in court %d]\n", info->name, info->id, local_played, kids_in_court);
        kids_in_court--;
        if(info->has_ball){
            pthread_mutex_lock(&court);
            ball_owners_in_court--;
            // printf("%s (%d): Sai com minha bola. %d pessoas com bola na quadra\n",
            // info->name, info->id, ball_owners_in_court);
            if(ball_owners_in_court == 0){
                // Nao tem mais crianças com bola na quadra
                // Entao todos tem que sair
                // printf("%s (%d): Mandei geral sair\n", info->name, info->id);
                for(int i = 0; i < KIDS; i++){
                    if(i != info->id){
                        set_my_state(i, COME_HOME);
                        sem_post(&child_pb[i]);
                    }
                }
                kids_waiting_to_play = -1;
            }
            pthread_mutex_unlock(&court);
        }
        
        if(local_played && kids_in_court > 0){
            pthread_mutex_lock(&court);
            pthread_mutex_lock(&states);
            if(kids_waiting_to_play != -1){
                // Ve se alguem estava esperando para brincar
                // E coloca essa criança para brincar
                // printf("%s (%d): Colocando %d para brincar\n", info->name, info->id, kids_waiting_to_play);
                set_my_state(kids_waiting_to_play, LETS_PLAY);
                played[kids_waiting_to_play] = true;
                kids_waiting_to_play = -1;
            } else {
                // Ninguem mais queria brincar
                // Porem o numero de jogadores agora eh impar
                // Entao tem que tirar alguem
                for(int i = info->id + 1; i < KIDS; i = (i+1)%KIDS){
                    if(get_my_state(i) == LETS_PLAY){
                        // printf("%s (%d): Tirando (%d) da brincadeira\n", info->name, info->id, i);
                        set_my_state(i, WANNA_PLAY);
                        played[i] = false;
                        sem_post(&child_pb[i]);
                        break;
                    }
                }
            }
            pthread_mutex_unlock(&states);
            pthread_mutex_unlock(&court);
        } else {
            if(kids_waiting_to_play == info->id){
                kids_waiting_to_play = -1;
            }
        }
        pthread_mutex_unlock(&states);

        // Criança fazendo coisas
        sleep(rand()%20 + 5);
    }
}

char* pick_name(int i){
    char* names[24] = {
        "Jonas", "Gustavo", "Marta", "Miranda",
        "Luna", "Fernanda", "Felipe", "Matheus",
        "Vitor", "Thiago", "Gabriel", "Andre",
        "Marcos", "Barbara", "Beatriz", "Sarah",
        "Thais", "Bruna", "Ana", "Vinicius",
        "Escanor", "Merlin", "Elizabeth", "Luke"
    };
    return names[i];
}

int main(){
    // Seed rand
    srand(time(NULL));
    // Inicializa mutex recursivo
    pthread_mutexattr_init(&states_attr);
    pthread_mutexattr_settype(&states_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&states, &states_attr);
    // Inicializa informações da quadra
    ball_owners_in_court = 0;
    kids_in_court = 0;
    kids_waiting_to_play = -1;
    // Informações das crianças
    child_t kids_info[KIDS];
    for(int i = 0; i < KIDS; i++){
        // Inicializa os semaforos das crianças
        sem_init(&child_pb[i], 0, 0);
        child_state[i] = -1;
        played[i] = false;
        // Inicializa as informações das crianças
        kids_info[i].id = i;
        kids_info[i].name = pick_name(i);
        kids_info[i].has_ball = i%BALL == 0 ? true : false;
    }
    // Inicializa a informação das crianças
    // E as threads das crianças
    pthread_t kids[KIDS];
    for(int i = 0; i < KIDS; i++){
        pthread_create(&kids[i], NULL, child, (void*) &kids_info[i]);
    }
    // Mostra as informações na tela
    while(true){
        system("clear");
        pthread_mutex_lock(&court);
        pthread_mutex_lock(&states);
        int kids_playing = 0;
        char* playing[KIDS];
        int idx = 0;
        for(int i = 0; i < KIDS; i++){
            if(child_state[i] == LETS_PLAY){
                kids_playing++;
                playing[idx++] = kids_info[i].name;
            }
        }
        printf(
            "%23s: %02d %23s: %02d %23s: %d\n", 
            "Kids playing", kids_playing,
            "Kids in the court", kids_in_court,
            "Kids with a ball", ball_owners_in_court
        );
        
        printf("KIDS WAITING TO PLAY\n");
        if(kids_waiting_to_play != -1){
            printf("%10s\n", kids_info[kids_waiting_to_play].name);
        } else {
            printf("All children happy :D\n");
        }

        printf("KIDS PLAYING:\n");
        for(int i = 0; i < kids_playing-1; i+= 2){
            printf("%10s\t%10s\n", playing[i], playing[i+1]);
        }

        if(kids_playing > 0){
            // Get id of children playing
            int playing_idx[KIDS];
            idx = 0;
            for(int i = 0; i < kids_playing; i++){
                if(child_state[i] == LETS_PLAY){
                    playing_idx[idx] = i;
                    idx++;
                }
            }
            // Chance that mom will call some kid home
            int come_home = rand()%100;
            if(come_home >= 70) {
                // Decide who will not play anymore
                int poor_kid = rand()%kids_playing;
                poor_kid = playing_idx[poor_kid];
                printf("%s, vem pra casa agora!\n", kids_info[poor_kid].name);
                child_state[poor_kid] = COME_HOME;
                sem_post(&child_pb[poor_kid]);
            }
        }
        pthread_mutex_unlock(&court);
        pthread_mutex_unlock(&states);
        sleep(1);
    }
}