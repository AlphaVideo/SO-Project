#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
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
socklen_t serverlen;

void init_socket(char* path);
void execThreads(int nThreads);
char* receive_command(struct sockaddr_un *clientAddr, socklen_t clilen);
void send_result(struct sockaddr_un *clientAddr, socklen_t clilen, int res);
void close_socket(char* path);

void applyCommands(){

    /* Lookup function requires it's own external list */
    pthread_rwlock_t *lookupLocks[INODE_TABLE_SIZE] = {NULL};

    while(1)
    {
        struct sockaddr_un clientAddr;
        socklen_t clilen = sizeof(struct sockaddr_un);

        const char* command = receive_command(&clientAddr, clilen);
        
        if (command == NULL || strcmp(command, "") == 0){
            continue;
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
        int r; /* Result to send to client */
        switch (token) {
            case 'c':
                /* For create, we only use the first character for the type */
                switch (typeOrPath[0]) {
                    case 'f':
                        printf("Create file: %s\n", name);
                        r = create(name, T_FILE);
                        send_result(&clientAddr, clilen, r);
                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);
                        r = create(name, T_DIRECTORY);
                        send_result(&clientAddr, clilen, r);
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
                send_result(&clientAddr, clilen, searchResult);
                break;
            case 'd':
                printf("Delete: %s\n", name);
                r = delete(name);
                send_result(&clientAddr, clilen, r);
                break;
            case 'm':
                /* For m, we need to use typeOrPath as a string */
                printf("Move: %s to %s\n", name, typeOrPath);
                validPath = lookup(name, lookupLocks); 
                if (validPath < 0)
                {
                    printf("Error: origin pathname does not exist.\n");
                    lockListClear(lookupLocks);
                    r = FAIL;
                    send_result(&clientAddr, clilen, r);
                    break;
                }
                lockListClear(lookupLocks);
                r = move(name, typeOrPath);
                send_result(&clientAddr, clilen, r);
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    exit(EXIT_FAILURE);
}

/* Function that handles the initialization and closing of the threads. 
Threads created enter the client wait cycle. */
void execThreads(int nThreads)
{
    int i;
    pthread_t *tids = (pthread_t*) malloc((nThreads) * sizeof(pthread_t));

    /* Thread management */

    /*Starts threads */
    for(i = 0; i < nThreads; i++)
    {
        if(pthread_create(&tids[i], NULL, (void*) applyCommands, NULL) != 0)
        {
            fprintf(stderr, "Error: couldn't create consumer thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    /*Joins child threads*/
    for(i = 0; i < nThreads; i++)
    {
        if(pthread_join(tids[i], NULL) != 0)
        {
            fprintf(stderr, "Error: couldn't join consumer thread.\n");
            exit(EXIT_FAILURE);
        }  
    }

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
        serverlen = 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    serverlen = SUN_LEN(addr);

    if (bind(sockfd, (struct sockaddr *) addr, serverlen) < 0) 
    {
        fprintf(stderr, "TecnicoFS: bind error\n");
        exit(EXIT_FAILURE);
    }
}

char* receive_command(struct sockaddr_un *clientAddr, socklen_t clilen)
{
    char in_buffer[MAX_INPUT_SIZE];
    char* command;
    int c; /* Number of bytes read */

    c = recvfrom(sockfd, in_buffer, sizeof(in_buffer)-1, 0, (struct sockaddr *) clientAddr, &clilen);
    if (c <= 0) 
        return NULL; //Failed to read or read 0
    //Preventivo, caso o cliente nao tenha terminado a mensagem em '\0', 
    in_buffer[c]='\0';

    command = malloc(sizeof(char) * strlen(in_buffer));
    strcpy(command, in_buffer);
    return command;
}

/* Sends the result of the operation to the client that gave the command */
void send_result(struct sockaddr_un *clientAddr, socklen_t clilen, int res)
{
    int result[1];
    result[0] = res;
    if (sendto(sockfd, result, sizeof(result)+1, 0, (struct sockaddr *) clientAddr, clilen) < 0) 
    {
        fprintf(stderr, "Server: sendto error\n");
        exit(EXIT_FAILURE);
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

    execThreads(numberThreads);

    close_socket(argv[2]);

    /* release allocated memory */
    destroy_fs();
    exit(EXIT_SUCCESS);
}


