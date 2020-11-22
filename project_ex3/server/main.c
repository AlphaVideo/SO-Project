#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "fs/operations.h"
#include "lock.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

int numberThreads = 0;

/* Socket variables */
int sockfd;
struct sockaddr_un serverAddr;
socklen_t addrlen;

/* Input buffer and it's counter and index trackers */
char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int insertPtr = 0;
int removePtr = 0;
int inputFinished = 0;
int consumersFinished = 0;

/* Global Lock and it's Conds */
pthread_mutex_t commandlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t canInsert= PTHREAD_COND_INITIALIZER;
pthread_cond_t canRemove = PTHREAD_COND_INITIALIZER;

void init_socket(char* path);
void execThreads(char* input);
void receive_command();
void close_socket(char* path);

int insertCommand(char* data) {
    commandLockLock(commandlock);

    while(numberCommands == MAX_COMMANDS) {
        if(pthread_cond_wait(&canInsert, &commandlock) != 0)
        {
            fprintf(stderr, "Error: failed to wait for canInsert.\n");
            exit(EXIT_FAILURE);
        }
    }
    strcpy(inputCommands[insertPtr++], data);

    /* Keeps track of the total item counter and insertion index */
    if(insertPtr == MAX_COMMANDS)
        insertPtr = 0;
    numberCommands++;

    if(pthread_cond_signal(&canRemove) != 0)
    {
        fprintf(stderr, "Error: couldn't signal cond canRemove.\n");
        exit(EXIT_FAILURE);
    }
    commandLockUnlock(commandlock);
    return 1;
}

char* removeCommand() {
    /* Locking and signalling is done externally to protect later scan of the returned address */
    char* command;

    while(numberCommands == 0 && !inputFinished){
        if(pthread_cond_wait(&canRemove, &commandlock) != 0)
        {
            fprintf(stderr, "Error: failed to wait for canRemove.\n");
            exit(EXIT_FAILURE);
        }
    }
    command = inputCommands[removePtr++];

    /* Keeps track of the total item counter and insertion index */
    if(removePtr == MAX_COMMANDS)
        removePtr = 0;
    numberCommands--;

    return command; 
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(char* input_file){
    char line[MAX_INPUT_SIZE];
    char* input = (char*) input_file;

    /* Opens the input file in read mode */
    FILE *in = fopen(input, "r");
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

            case 'm':
                if(numTokens != 3)
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
    
    if(fclose(in) != 0)
    {
        fprintf(stderr, "Error: input file could not be closed.\n");
        exit(EXIT_FAILURE);
    }

    /* Tells consumer threads that if numberCommands == 0, they can end */
    inputFinished = 1;
    
    /* Prevents rare case where consumers sneak into the wait while inputFinished is updated*/
    while(consumersFinished != (numberThreads -1))
    {
        if(pthread_cond_broadcast(&canRemove) != 0)
        {
            fprintf(stderr, "Error: couldn't broadcast cond canRemove.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void applyCommands(){

    /* Lookup function requires it's own external list */
    pthread_rwlock_t *lookupLocks[INODE_TABLE_SIZE] = {NULL};
    while ((numberCommands > 0) || !inputFinished){
        
        commandLockLock(commandlock);

        char commandCopy[MAX_INPUT_SIZE];
        const char* command = removeCommand();
        
        if (command == NULL || strcmp(command, "") == 0){
            commandLockUnlock(commandlock);
            continue;
        }
        else
            strcpy(commandCopy, command);

        if(pthread_cond_signal(&canInsert) != 0)
        {
            fprintf(stderr, "Error: couldn't signal cond canInsert.\n");
            exit(EXIT_FAILURE);
        }
        commandLockUnlock(commandlock);

        char token;
        char name[MAX_INPUT_SIZE];
        char typeOrPath[MAX_INPUT_SIZE];
        int numTokens = sscanf(commandCopy, "%c %s %s", &token, name, typeOrPath);

        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int searchResult;
        int validPath;
        switch (token) {
            case 'c':
                /* For create, we only use the first character for the type */
                switch (typeOrPath[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        create(name, T_FILE);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        create(name, T_DIRECTORY);
                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 
                searchResult = lookup(name, lookupLocks);
                
                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                lockListClear(lookupLocks);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                delete(name);
                break;
            case 'm':
                /* For m, we need to use typeOrPath as a string */
                printf("Move: %s to %s\n", name, typeOrPath);
                validPath = lookup(name, lookupLocks); 
                if (validPath < 0)
                {
                    printf("Error: origin pathname does not exist.\n");
                    lockListClear(lookupLocks);
                    break;
                }
                lockListClear(lookupLocks);
                move(name, typeOrPath);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    consumersFinished++;
    return;
}

/* Function that handles the initialization and closing of the threads. 
Also applies the FS commands and handles execution time. */
void execThreads(char* input)
{
    int i;
    numberThreads++; /* Input thread always exists */
    pthread_t *tids = (pthread_t*) malloc((numberThreads) * sizeof(pthread_t));

    struct timeval start, stop;
    double elapsed;    

    gettimeofday(&start, NULL); /* Gets the starting time.*/

    /* Thread management */

    /*Starts input parsing */
    for(i = 0; i < numberThreads; i++)
    {
        /* Starts input insertion on first cycle*/
        if(i == 0)
        {
            if(pthread_create(&tids[i], NULL, (void*) processInput, (void*) input) != 0)
            {
                fprintf(stderr, "Error: couldn't create input thread.\n");
                exit(EXIT_FAILURE);
            }
        }
        /* Creates slave thread on every other cycle*/
        else
        {
            if(pthread_create(&tids[i], NULL, (void*) applyCommands, NULL) != 0)
            {
                fprintf(stderr, "Error: couldn't create consumer thread.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    /*Joins child threads*/
    for(i = 0; i < numberThreads; i++)
    {
        if(i == 0)
        {
            if(pthread_join(tids[i], NULL) != 0)
            {
                fprintf(stderr, "Error: couldn't join input insertion thread.\n");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if(pthread_join(tids[i], NULL) != 0)
            {
                fprintf(stderr, "Error: couldn't join consumer thread.\n");
                exit(EXIT_FAILURE);
            }  
        }
    }
    
    gettimeofday(&stop, NULL); 

    /* Calculates the time elapsed */
    elapsed = ((double) stop.tv_sec + (double) (stop.tv_usec/1000000.0)) - ((double) start.tv_sec + (double) (start.tv_usec/1000000.0));
    printf("TecnicoFS completed in %.4f seconds.\n", elapsed);
    free(tids);
}

/* Creates and binds a new socket with the path given by main */
void init_socket(char* path)
{
    struct sockaddr_un *addr = &serverAddr;
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) 
    {
        fprintf(stderr, "TecnicoFS: can't open socket at %s\n", path);
        exit(EXIT_FAILURE);
    }
    unlink(path);

    if (addr == NULL)
        addrlen = 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    addrlen = SUN_LEN(addr);

    if (bind(sockfd, (struct sockaddr *) addr, addrlen) < 0) 
    {
        fprintf(stderr, "TecnicoFS: bind error\n");
        exit(EXIT_FAILURE);
    }
}

void receive_command()
{
    while (1) 
    {
        struct sockaddr_un clientAddr;
        char in_buffer[MAX_INPUT_SIZE];
        int c; /* Number of bytes read */

        addrlen = sizeof(struct sockaddr_un);
        c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *)&clientAddr, &addrlen);
        if (c <= 0) 
            continue; //Failed to read or read 0
        //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
        in_buffer[c]='\0';
        
        /* Prints command from client to stdout */
        printf("%s", in_buffer);
    }
}

/* Closes the socket with the given path */
void close_socket(char* path)
{
    close(sockfd);
    unlink(path);
}


int main(int argc, char* argv[]) 
{
    /* Argument parsing */
    
    if(argc != 3)
    {
        fprintf(stderr, "Error: number of given arguments incorrect.\n");
        exit(EXIT_FAILURE);
    }
    
    if(atoi(argv[1]) <= 0)
    {
        fprintf(stderr, "Error: number of threads given incorrect.\n");
        exit(EXIT_FAILURE); 
    }

    
    numberThreads = atoi(argv[1]);

    /* init filesystem */
    init_fs();

    /* Init datagram socket */
    init_socket(argv[2]);

    receive_command();

    // /* Process input and create desired threads*/
    // execThreads(argv[1]);

    // FILE *out = fopen(argv[2], "w");

    // if(out == NULL)
    // {
    //     fprintf(stderr, "Error: output file couldn't be created.\n");
    //     exit(EXIT_FAILURE);
    // }

    // print_tecnicofs_tree(out);

    // if(fclose(out) != 0)
    // {
    //     fprintf(stderr, "Error: input file could not be closed.\n");
    //     exit(EXIT_FAILURE);
    // }

    // /* Lock destruction */
    // destroyGlobalLock();

    close_socket(argv[2]);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}


