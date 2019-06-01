/* Criado por Giovanni M Guidini - 16/0122660
    Em um condomínio existem N crianças. M (M < N) dessas crianças possuem uma bola.

    As crianças brincam de bola na quadra do condomínio. Crianças só podem brincar se
    alguma entre elas possuem a bola, e somente se existir um número PAR de crianças na
    quadra (se houver um número IMPAR a última criança precisa esperar alguém sair, ou
    que mais alguém chegue).

    Eventualmente as mães das crianças as chamam para casa, e nesse momento ela deve parar
    de brincar. Se a criança chamada é a única dona de uma bola, a brincadeira acaba.
    
    Este código só funciona em sistemas UNIX.
    Compile com a flag -pthread
*/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>

#define KIDS 24
#define BALL 8

#define LETS_PLAY 0
#define COME_HOME 1
#define WANNA_PLAY 3

#define GREEN "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"
#define CYAN "\x1b[36m"
#define RED "\x1b[31m"

pthread_mutex_t court = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t events = PTHREAD_MUTEX_INITIALIZER;
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
    char* color;
} child_t;

// Eventos
struct {
    char event[10][100];
    int idx;
    bool has_event;
} Events;

// Add new event
void add_event(char ev[]){
    pthread_mutex_lock(&events);
    Events.has_event = true;
    strcpy(Events.event[Events.idx], ev);
    Events.idx = (Events.idx + 1)%10;
    pthread_mutex_unlock(&events);
}

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
        char event_to_add[100];
        strcpy(event_to_add, info->color);
        strcat(event_to_add, info->name);
        add_event(strcat(event_to_add, ": Entrei na quadra para brincar :)\n"));
        kids_in_court++;
        played[info->id] = false;
        set_my_state(info->id, WANNA_PLAY);
        pthread_mutex_unlock(&court);
        // Criança esta na quadra
        // Precisa aguardar mensagens:
        // Se o numero de criancas eh par, para poder jogar
        // Se a mae esta chamando de volta
        // Se o dono da bola foi embora
        int s = get_my_state(info->id);
        while(s != COME_HOME){
            pthread_mutex_lock(&states); 
            if(s == WANNA_PLAY && kids_waiting_to_play != -1 && kids_waiting_to_play != info->id){
                // Criança quer brincar e existe um numero impar de crianças que tambem querem
                // Libera a outra crinaça que está esperando
                set_my_state(info->id, LETS_PLAY);
                set_my_state(kids_waiting_to_play, LETS_PLAY);
                played[kids_waiting_to_play] = true;
                played[info->id] = true;
                kids_waiting_to_play = -1;
            } else if(s == WANNA_PLAY && kids_waiting_to_play == -1){
                kids_waiting_to_play = info->id;
                played[info->id] = false;                
            }
            pthread_mutex_unlock(&states); 
            // Espera alguma ação que precisa de resposta
            sem_wait(&child_pb[info->id]);
            s = get_my_state(info->id);     
        }
        // Clean up antes de sair
        pthread_mutex_lock(&states);
        bool local_played = played[info->id];
        kids_in_court--;
        if(info->has_ball){
            pthread_mutex_lock(&court);
            ball_owners_in_court--;
            // info->name, info->id, ball_owners_in_court);
            if(ball_owners_in_court == 0){
                pthread_mutex_lock(&states);
                // Nao tem mais crianças com bola na quadra
                // Entao todos tem que sair
                int i;
                for(i = 0; i < KIDS; i++){
                    if(i != info->id && get_my_state(i) != COME_HOME){
                        set_my_state(i, COME_HOME); // Manda todo mundo sair
                        played[i] = false;          // Eles saem sem alterar nada
                        sem_post(&child_pb[i]);
                    }
                }
                char event_to_add[100];
                strcpy(event_to_add, YELLOW);
                strcat(event_to_add, info->name);
                add_event(strcat(event_to_add, ": ACABOU A BRINCADEIRA, PESSOAL :/\n"));
                kids_waiting_to_play = -1;
                played[info->id] = false;
                local_played = false;
                pthread_mutex_unlock(&states);
            }
            pthread_mutex_unlock(&court);
        }
        
        if(local_played && kids_in_court > 0){
            pthread_mutex_lock(&court);
            pthread_mutex_lock(&states);
            if(kids_waiting_to_play != -1){
                // Ve se alguem estava esperando para brincar
                // E coloca essa criança para brincar
                set_my_state(kids_waiting_to_play, LETS_PLAY);
                played[kids_waiting_to_play] = true;
                kids_waiting_to_play = -1;
            } else {
                // Ninguem mais queria brincar
                // Porem o numero de jogadores agora eh impar
                // Entao tem que tirar alguem
                int i;
                for(i = info->id + 1; i < KIDS; i = (i+1)%KIDS){
                    if(get_my_state(i) == LETS_PLAY){
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
        if(!info->has_ball)
            sleep(rand()%20 + 5);
        else 
            sleep(rand()%20 + 10);
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
    Events.idx = 0;
    Events.has_event = false;
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
    int i;
    for(i = 0; i < KIDS; i++){
        // Inicializa os semaforos das crianças
        sem_init(&child_pb[i], 0, 0);
        child_state[i] = -1;
        played[i] = false;
        // Inicializa as informações das crianças
        kids_info[i].id = i;
        kids_info[i].name = pick_name(i);
        kids_info[i].has_ball = i%BALL == 0 ? true : false;
        kids_info[i].color = i%BALL == 0 ? BLUE : GREEN;
    }
    // Inicializa a informação das crianças
    // E as threads das crianças
    pthread_t kids[KIDS];
    for(i = 0; i < KIDS; i++){
        pthread_create(&kids[i], NULL, child, (void*) &kids_info[i]);
    }
    // Mostra as informações na tela
    while(true){
        system("clear");
        pthread_mutex_lock(&court);
        pthread_mutex_lock(&states);
        int kids_playing = 0;
        int playing[KIDS];
        int idx = 0;
        for(i = 0; i < KIDS; i++){
            if(child_state[i] == LETS_PLAY){
                kids_playing++;
                playing[idx++] = kids_info[i].id;
            }
        }
        printf(
            "%23s: %02d\t%23s: %02d\t" BLUE "%23s: %d\n\n" RESET, 
            "Galera brincando", kids_playing,
            "Galera na quadra", kids_in_court,
            "Donos de bola na quadra", ball_owners_in_court
        );
        
        printf("QUEM TA ESPERANDO PARA BRINCAR:\n");
        if(kids_waiting_to_play != -1){
            printf("%s%10s\n" RESET, kids_info[kids_waiting_to_play].color, kids_info[kids_waiting_to_play].name);
        } else {
            printf(CYAN "Todo mundo brincando :D\n" RESET);
        }

        printf("QUEM TA BRINCANDO:\n");
        for(i = 0; i < kids_playing-1; i+= 2){
            printf( "%s%10s" RESET "\t%s%10s\n" RESET,
            kids_info[playing[i]].color, kids_info[playing[i]].name,
            kids_info[playing[i+1]].color, kids_info[playing[i+1]].name);
        }

        if(kids_playing > 0){
            // Get id of children playing
            int playing_idx[KIDS];
            idx = 0;
            for(i = 0; i < kids_playing; i++){
                if(child_state[i] == LETS_PLAY){
                    playing_idx[idx] = i;
                    idx++;
                }
            }
            // Chance that mom will call some kid home
            int come_home = rand()%100;
            if(come_home >= 55) {
                // Decide who will not play anymore
                int poor_kid = rand()%kids_playing;
                poor_kid = playing_idx[poor_kid];
                char event_to_add[100];
                strcpy(event_to_add, RED);
                strcat(event_to_add, kids_info[poor_kid].name);
                add_event(strcat(event_to_add, ", VEM PRA CASA AGORA!\n"));
                child_state[poor_kid] = COME_HOME;
                sem_post(&child_pb[poor_kid]);
            }
        }

        printf("ACONTECIMENTOS RECENTES:\n");
        pthread_mutex_lock(&events);
            if(Events.has_event){
                int c = 0;
                for(i = Events.idx; c < 10; i = (i+1)%10){
                    printf("%s" RESET, Events.event[i]);
                    c++;
                }
            } else {
                printf("Sem eventos\n");
            }
        pthread_mutex_unlock(&events);
        pthread_mutex_unlock(&court);
        pthread_mutex_unlock(&states);
        sleep(1);
    }
}
