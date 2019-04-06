/* Created by Gguidini, April 5th, 2019
*   This program simulates the readers/writers problem.
*
*   Basically you have a shared database (in this casa e .txt file) and many writers and readers
*   fighting over it.
*   Here, writers write to the file, readers read from the file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

pthread_mutex_t LOCK;           // Lock for the shared file
pthread_mutex_t READERS_LOCK;   // Lock for readers to get LOCK

int NUMBER_OF_READERS = 5;      // Number of readers to create
int NUMBER_OF_WRITERS = 2;      // Number of writers to create

float READERS_WAIT = 2.5;       // Time readers sleep before reading file
float WRITERS_WAIT = 5;         // Time writers sleep before writing to file

char* SHARED_FILE = "shared_db.txt";    // The shared resource

int reading = 0;                // Number of readers currently reading file

struct reader_info{
    long id;
    char color[9];
};
// Returns string with local time. Final \n included
// Format example: Fri Apr  5 23:36:49 2019
char* now(){
    time_t rawtime;
    struct tm * timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    return asctime(timeinfo);
}

// Returns a word from the list of words
char* get_word(int idx){
    char* words[26] = {
        "Alpha", "Bravo", "Charlie", "Echo", "Foxtrot",
        "Golf", "Hotel", "India", "Juliet", "Kilo",
        "Lima", "Mike", "November", "Oscar", "Papa",
        "Quebec", "Romeo", "Sierra", "Tango", "Uniform",
        "Victor", "Whiskey", "X-ray", "Yankee", "Zulu"
    };
    return words[idx%26];
}

// Writers write to the shared file every WRITERS_WAIT seconds.
// Writer output in file will be "[<WORD>] - <ID> <TIME>"
void* writer(void* id){
    // gets id and opens file for APPENDING
    long thread_id = (long) id;
    FILE* fd = fopen(SHARED_FILE, "a+");
    // writes all words in word bank, in order.
    for(int i = 0; i < 26; i++){
        printf(YELLOW "%ld - WRITING FILE AT TIME %s" RESET, thread_id, now());
        // CRITIC REGION
        pthread_mutex_lock(&LOCK);
        char* time = now(); // inside region to see if process is blocked.
        fprintf(fd, "[%s] - %ld at %s", get_word(i), thread_id, time);
        fflush(fd);     // makes sure change is saved in file
        pthread_mutex_unlock(&LOCK);
        // CRITIC REGION ENDS
        sleep(WRITERS_WAIT);
    }
    // work is done
    fclose(fd);
    pthread_exit(0);
}

// Reders will read from shared file every READERS_WAIT seconds.
// Readers print output in different colors, except YELLOW.
void* reader(void* info){
    // gets info, open file for READING
    struct reader_info *thread_info = (struct reader_info*) info;
    FILE* fd = fopen(SHARED_FILE, "r+");
    // init string to store file line
    char str[51];
    str[50] = '\0';
    for(int i = 0; i < 50; i++){
        // CRITIC REGION - JUST READERS
        pthread_mutex_lock(&READERS_LOCK);
        reading++;
        // Many readers can read at the same time,
        // but they must prevent writer from writing while they read.
        if( reading == 1){
            // CRITIC REGION - READERS READING
            pthread_mutex_lock(&LOCK);
        }
        pthread_mutex_unlock(&READERS_LOCK);
        // CRITIC REGION ENDS - JUST READERS
        // reading file
        while (fgets(str, 50, fd) != NULL){
            printf("%s%ld - READING LINE\n" RESET, thread_info->color, thread_info->id);
        }
        printf("%s%ld's last line: %s" RESET, thread_info->color, thread_info->id, str);
        // CRITIC REGION - JUST READERS
        pthread_mutex_lock(&READERS_LOCK);
        reading--;
        if( reading == 0){
            pthread_mutex_unlock(&LOCK);
            // CRITIC REGION ENDS - NO READERS READING
        }
        pthread_mutex_unlock(&READERS_LOCK);
        // CRITIC REGION ENDS - JUST READERS
        rewind(fd);
        sleep(READERS_WAIT);
    }
    fclose(fd);
    pthread_exit(0);
}

int main() {
    // init locks
    pthread_mutex_init(&LOCK, NULL);
    pthread_mutex_init(&READERS_LOCK, NULL);
    // the workers
    pthread_t writers[NUMBER_OF_WRITERS];
    pthread_t readers[NUMBER_OF_WRITERS];
    // readers info - color and id
    struct reader_info infos[NUMBER_OF_READERS];
    for(int i = 0; i < NUMBER_OF_READERS; i++){
        infos[i].id = i + 50;
        switch (i%5) {
            case 0:
                strcpy(infos[i].color, RED);   
                break;
            case 1:
                strcpy(infos[i].color, GREEN);   
                break;
            case 2:
                strcpy(infos[i].color, BLUE);   
                break;
            case 3:
                strcpy(infos[i].color, MAGENTA);   
                break;
            case 4:
                strcpy(infos[i].color, CYAN);   
                break;
        }
    }
    // Cleans shared file (previous execution may have already created file)
    FILE* fd = fopen(SHARED_FILE, "w+");
    fclose(fd);
    // create writers (i is long so it has same number of bytes as void*)
    for(long i = 0; i < NUMBER_OF_WRITERS; i++){
        pthread_create(&writers[i], NULL, writer, (void*) i);
    }
    // create readers
    for(int i = 0; i < NUMBER_OF_READERS; i++){
        pthread_create(&readers[i], NULL, reader, (void*) &infos[i]);
    }
    // wait workers finish
    for(int i = 0; i < NUMBER_OF_WRITERS; i++){
        pthread_join(writers[i], NULL);
    }
    for(int i = 0; i < NUMBER_OF_READERS; i++){
        pthread_join(readers[i], NULL);
    }
    // bye bye
    puts("Exiting");
    return 0;
}