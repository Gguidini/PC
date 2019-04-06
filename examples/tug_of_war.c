#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
    #define CLEAR "cls"
#else
    #define CLEAR "clear"
#endif

#define RESET   "\x1b[0m"

#define RED     "\x1b[31m"
#define BLUE    "\x1b[34m"
#define BLACK   "\x1b[30m"
#define WHITE   "\x1b[37m"

#define BKGD_RED "\x1b[41m"
#define BKGD_RED_ACTIVE "\x1b[101m"
#define BKGD_BLUE    "\x1b[44m"
#define BKGD_BLUE_ACTIVE    "\x1b[104m"
#define BKGD_BLACK   "\x1b[40m"
#define BKGD_WHITE   "\x1b[47m"

int PLAYER_IN_TEAM = 2;             // Number of players in each team

pthread_mutex_t LOCK = PTHREAD_MUTEX_INITIALIZER;   // Lock fro shared info

// Defines some information to be displayed in screen about the game
typedef struct {
    bool* team_a;
    bool* team_b;
    time_t start_time;
} tug_of_war;

typedef struct {
    long id;
    int team;
    double rest_time;
} player_info;

long long tug = 0;                  // The tug of war
long long POINTS_TO_WIN = 1000000000;    // Number of points for a team to win

tug_of_war GAME;        // info panel
int WINNER = 2;         // The winning team (0 red, 1 blue)

bool update(int team){
    if(team == 0){
        tug--;
    } else{
        tug++;
    }
    if (tug <= -POINTS_TO_WIN) WINNER = 0;
    else if (tug >= POINTS_TO_WIN) WINNER = 1;
    return (tug <= -POINTS_TO_WIN || tug >= POINTS_TO_WIN);
}

void* player(void* args){
    player_info* info = (player_info*) args;
    bool end_of_game = false;
    while(!end_of_game){
        if(info->team == 0){
            GAME.team_a[info->id] = true;
        } else {
            GAME.team_b[info->id] = true;
        }
        for(int i = 0; i < 10000000; i++){
            pthread_mutex_lock(&LOCK);
            end_of_game = update(info->team);
            pthread_mutex_unlock(&LOCK);
        }
        if(info->team == 0){
            GAME.team_a[info->id] = false;
        } else {
            GAME.team_b[info->id] = false;
        }
        sleep(info->rest_time);
    }
    pthread_exit(0);
}

void display_info(){
    // Current value of tug
    printf("\n%49s: %lld\n", "CURRENT VALUE", tug);
    int bar = abs((tug / (double) POINTS_TO_WIN) * 50);
    // Display colored bar with teams progress
    // first team
    if(tug < 0){
        printf(BKGD_RED);
        for(int i = 0; i < (50 - bar); i++) putchar(' ');
        printf(BKGD_RED_ACTIVE);
        for(int i = 0; i < bar; i++) putchar(' ');
        printf(RESET);
    }
    else {
        printf(BKGD_RED);
        for(int i = 0; i < 50; i++) putchar(' ');
        printf(RESET);
    }
    // turning point
    printf(BKGD_WHITE " " RESET);
    // second team
    if(tug > 0){
        printf(BKGD_BLUE_ACTIVE);
        for(int i = 0; i < bar; i++) putchar(' ');
        printf(BKGD_BLUE);
        for(int i = 0; i < (50 - bar); i++) putchar(' ');
        printf(RESET);
    }
    else {
        printf(BKGD_BLUE);
        for(int i = 0; i < 50; i++) putchar(' ');
        printf(RESET);
    }
    printf("\n");
    // Display if players are active or not
    for(int i = 0; i < PLAYER_IN_TEAM; i++){
        bool a = GAME.team_a[i];
        bool b = GAME.team_b[i];
        if(a){
            printf(BKGD_RED BLACK "%s", "Team RED Player");
        } else {
            printf(BKGD_BLACK RED "%s", "Team RED Player");
        }
        for(int i = 0; i < 35; i++) putchar(' ');
        printf(RESET);
        putchar(' ');
        if(b){
            printf(BKGD_BLUE BLACK "%s", "Team BLUE Player");
        } else {
            printf(BKGD_BLACK BLUE "%s", "Team BLUE Player");
        }
        for(int i = 0; i < 34; i++) putchar(' ');
        printf(RESET);
        printf("\n");
    }
}

int main(){
    // seeds the random generator
    srand(time(NULL));
    // Player is active or not
    bool team_a[PLAYER_IN_TEAM];
    bool team_b[PLAYER_IN_TEAM];
    // Player info
    player_info info_a[PLAYER_IN_TEAM];
    player_info info_b[PLAYER_IN_TEAM];
    // The players
    pthread_t players_a[PLAYER_IN_TEAM];
    pthread_t players_b[PLAYER_IN_TEAM];
    // Generate info. All Players start as NOT active.
    // Max rest time is 5s
    for(int i = 0; i < PLAYER_IN_TEAM; i++){
        team_a[i] = false;
        team_b[i] = false;
        info_a[i].id = i;
        info_b[i].id = i;
        info_a[i].team = 0;
        info_b[i].team = 1;
        info_a[i].rest_time = (rand() % 5 + (rand() % 100) * 0.01); 
        info_b[i].rest_time = (rand() % 5 + (rand() % 100) * 0.01);
    }
    // Displays players info. Waits for continue signal
    for(int i = 0; i < PLAYER_IN_TEAM; i++){
        printf( RED "Player %d: rest time: %.2lf\n" RESET, i, info_a[i].rest_time);
        printf( BLUE "Player %d: rest time: %.2lf\n" RESET, i, info_b[i].rest_time);
    }
    getchar();
    // Inits the info panel
    GAME.team_a = team_a;
    GAME.team_b = team_b;
    GAME.start_time = time(NULL);
    // start players
    for(int i = 0; i < PLAYER_IN_TEAM; i++){
        pthread_create(&players_a[i], NULL, player, (void*) &info_a[i]);
        pthread_create(&players_b[i], NULL, player, (void*) &info_b[i]);
    }
    
    // main updates game until it is over
    while(WINNER == 2){
        pthread_mutex_lock(&LOCK);
        display_info();
        pthread_mutex_unlock(&LOCK);
        sleep(0.6);
        system(CLEAR);
    }

    if(WINNER == 0){
        printf( BKGD_RED_ACTIVE BLACK "%50s\n", "TEAM RED WINS!!!" RESET);
    } else {
        printf( BKGD_BLUE_ACTIVE BLACK "%50s\n", "TEAM BLUE WINS!!!" RESET);
    }
}
