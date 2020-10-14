#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include "fs/operations.h"
#include "sync.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

/* Global Locks */
pthread_mutex_t commandlock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mlock = PTHREAD_MUTEX_INITIALIZER;
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void execThreads(int sync);
void destroy_locks();

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char* input_file){
    char line[MAX_INPUT_SIZE];

    /* Opens the input file in read mode */
    FILE *in = fopen(input_file, "r");
    if (in == NULL)
    {
        fprintf(stderr, "Error: input file does not exist.\n");
        exit(EXIT_FAILURE);
    }

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), in)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case '#':
                break;
            
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(in);
}

void applyCommands(int *sync){

    int syncStrat = *sync; /*Removes value from pointer*/

    while (numberCommands > 0){

        commandLockLock(syncStrat);
        
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        
        commandLockUnlock(syncStrat);

        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE, syncStrat);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY, syncStrat);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name, syncStrat);
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name, syncStrat);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
}

/* Function that handles the initialization and closing of the threads. 
Also applies the FS commands and handles execution time. */
void execThreads(int sync)
{
    int i;
    pthread_t *tids = (pthread_t*) malloc(numberThreads * sizeof(pthread_t));
    struct timeval start, stop;
    double elapsed;    

    gettimeofday(&start, NULL); /* Gets the starting time.*/

    /* Thread management */
    
    for(i = 0; i < numberThreads; i++)
    {
        if(pthread_create(&tids[i], NULL, (void*) applyCommands, (void*) &sync) != 0)
        {
            fprintf(stderr, "Error: couldn't create thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < numberThreads; i++)
    {
        if(pthread_join(tids[i], NULL) != 0)
        {
            fprintf(stderr, "Error: couldn't join thread.\n");
            exit(EXIT_FAILURE);
        }
    }


    gettimeofday(&stop, NULL); 

    /* Calculates the time elapsed */
    elapsed = ((double) stop.tv_sec + (double) (stop.tv_usec/1000000.0)) - ((double) start.tv_sec + (double) (start.tv_usec/1000000.0));
    printf("TecnicoFS completed in %.4f seconds.\n", elapsed);

    free(tids);
}

void destroy_locks()
{
    if (pthread_mutex_destroy(&commandlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy command lock.");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_mutex_destroy(&mlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy command lock.");
        exit(EXIT_FAILURE);
    }

    if (pthread_rwlock_destroy(&rwlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy command lock.");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) 
{
    /* Argument parsing */
    
    if(argc != 5)
    {
        fprintf(stderr, "Error: number of given arguments incorrect.\n");
        exit(EXIT_FAILURE);
    }
    
    if(atoi(argv[3]) == 0)
    {
        fprintf(stderr, "Error: number of threads given incorrect.\n");
        exit(EXIT_FAILURE); 
    }

    /* Sync strategy parse */
    int syncStrat;
    if (strcmp(argv[4], "mutex") == 0)
        syncStrat = MUTEX;
    else if (strcmp(argv[4], "rwlock") == 0)
        syncStrat = RWLOCK;
    else if (strcmp(argv[4], "nosync") == 0)
        syncStrat = NOSYNC;
    else
    {
        fprintf(stderr,"Error: invalid synchronization strategy.\n");
        exit(EXIT_FAILURE);
    }
    
    numberThreads = atoi(argv[3]);

    /* init filesystem */
    init_fs();

    /* process input and print tree */
    processInput(argv[1]);
    execThreads(syncStrat);

    FILE *out = fopen(argv[2], "w");

    if(out == NULL)
    {
        fprintf(stderr, "Error: output file couldn't be created.");
        exit(EXIT_FAILURE);
    }

    print_tecnicofs_tree(out);
    fclose(out);

    /* Lock destruction */
    destroy_locks();

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}


