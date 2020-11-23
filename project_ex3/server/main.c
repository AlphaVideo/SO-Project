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

void init_socket(char* path);
void execThreads(char* input);
char* receive_command();
void close_socket(char* path);

void applyCommands(){

    /* Lookup function requires it's own external list */
    pthread_rwlock_t *lookupLocks[INODE_TABLE_SIZE] = {NULL};
        
    //Have client addr and client length that is reset every cycle
    //Keep it stored after receive_command to send the result back
    const char* command = receive_command();
    
    if (command == NULL || strcmp(command, "") == 0){
        return;
    }

    char token;
    char name[MAX_INPUT_SIZE];
    char typeOrPath[MAX_INPUT_SIZE];
    int numTokens = sscanf(command, "%c %s %s", &token, name, typeOrPath);

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
    return;
}

/* Function that handles the initialization and closing of the threads. 
Threads created enter the client wait cycle. */
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
        if(pthread_create(&tids[i], NULL, (void*) applyCommands, NULL) != 0)
        {
            fprintf(stderr, "Error: couldn't create consumer thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    /*Joins child threads*/
    for(i = 0; i < numberThreads; i++)
    {
        if(pthread_join(tids[i], NULL) != 0)
        {
            fprintf(stderr, "Error: couldn't join consumer thread.\n");
            exit(EXIT_FAILURE);
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

char* receive_command()
{
    struct sockaddr_un clientAddr;
    char in_buffer[MAX_INPUT_SIZE];
    char* command;
    int c; /* Number of bytes read */

    addrlen = sizeof(struct sockaddr_un);
    c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *)&clientAddr, &addrlen);
    if (c <= 0) 
        return NULL; //Failed to read or read 0
    //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
    in_buffer[c]='\0';
    
    /* Prints command from client to stdout */
    printf("%s", in_buffer);

    command = malloc(sizeof(char) * strlen(in_buffer));
    strcpy(command, in_buffer);
    return command;
    
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

    while(1)
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

    close_socket(argv[2]);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}


