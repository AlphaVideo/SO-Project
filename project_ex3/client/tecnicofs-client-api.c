#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

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
  /* TEMP */
  if (sendto(sockfd, filename, strlen(filename)+1, 0, (struct sockaddr *) &servAddr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  }
  return 0;
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

int tfsMount(char * sockPath) {

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

  unlink(CLIENT);
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
  close(sockfd);
  return 0;
}
