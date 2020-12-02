#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

/* Global command and result for operations */

char* command;
int* result;

/*Socket info and Server Socket info */

int sockfd;
char clientName[MAX_PATH_SIZE];
socklen_t servlen, clilen;
struct sockaddr_un servAddr, clientAddr;

/* Mounts the address info and returns the address length */
int addrSetup(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}


int tfsCreate(char *filename, char nodeType) {

  sprintf(command, "c %s %c", filename, nodeType);

  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: create sendto error");
    return -1;
  }

  if(recvfrom(sockfd, result, sizeof(result)-1, 0, 0, 0) < 0) {
    perror("client: create receive error");
    return -1;
  }

  return *result;
}

int tfsDelete(char *path) {
  sprintf(command, "d %s", path);

  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: delete sendto error");
    return -1;
  }

  if(recvfrom(sockfd, result, sizeof(result)-1, 0, 0, 0) < 0) {
    perror("client: delete receive error");
    return -1;
  }

  return *result;
}

int tfsMove(char *from, char *to) {
  sprintf(command, "m %s %s", from, to);

  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: move sendto error");
    return -1;
  }

  if(recvfrom(sockfd, result, sizeof(result)-1, 0, 0, 0) < 0) {
    perror("client: move receive error");
    return -1;
  }

  return *result;
}

int tfsLookup(char *path) {
  sprintf(command, "l %s", path);

  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: lookup sendto error");
    return -1;
  }

  if(recvfrom(sockfd, result, sizeof(result)-1, 0, 0, 0) < 0) {
    perror("client: lookup receive error");
    return -1;
  }

  if(*result >= 0)
    return 0;
  else
    return -1;
}

int tfsPrint(char* path) {

  sprintf(command, "p %s", path);

  if (sendto(sockfd, command, strlen(command)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: print sendto error");
    return -1;
  }

  if(recvfrom(sockfd, result, sizeof(result)-1, 0, 0, 0) < 0) {
    perror("client: print receive error");
    return -1;
  }

  return *result;
}

int tfsMount(char * sockPath) {

  command = malloc(sizeof(char)*MAX_INPUT_SIZE*2); /* Inits command */
  result = malloc(sizeof(int));
  char* PID_BUFFER = malloc(sizeof(char)*MAX_INPUT_SIZE);
  char* CLIENT_BUFFER = malloc(sizeof(char)*MAX_INPUT_SIZE);

  strcpy(CLIENT_BUFFER, CLIENT);

  /* Converts pid to a string */
  pid_t pid = getpid();
  sprintf(PID_BUFFER, "%d", pid);

  /*Client names must be unique*/
  CLIENT_BUFFER = strcat(CLIENT_BUFFER, PID_BUFFER);
  strcpy(clientName, CLIENT_BUFFER);

  /* Client socket init */
  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) 
    return -1;

  unlink(clientName);
  clilen = addrSetup(clientName, &clientAddr);
  if (bind(sockfd, (struct sockaddr *) &clientAddr, clilen) < 0)
    return -1;

  /* Server mount */
  servlen = addrSetup(sockPath, &servAddr);

  free(CLIENT_BUFFER);
  free(PID_BUFFER);
  return 0;
}

int tfsUnmount() {
  free(command);
  free(result);
  close(sockfd);
  unlink(clientName);
  return 0;
}
