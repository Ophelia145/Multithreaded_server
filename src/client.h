#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define FILE_PORT_MIN 49152
#define FILE_PORT_MAX 65535

typedef struct
{
  int file_port;
  int sockfd;
} ClientState;

void *file_server (void *arg);
void *recv_thread (void *arg);
int find_free_port ();