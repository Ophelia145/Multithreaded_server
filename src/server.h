#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 100

typedef struct
{
  int sockfd;
  char username[32];
  char ip[INET6_ADDRSTRLEN];
  int file_port;
  int authenticated;
} ClientInfo;

ClientInfo clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client (void *arg);
void broadcast_message (int sender_fd, const char *message, size_t len);