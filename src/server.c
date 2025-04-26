#include <netinet/ip6.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define port 8080

typedef struct
{
  int sockfd;
  struct sockaddr_in6 addr;
  char username[32];
  int authenticated;
} ClientInfo;

ClientInfo clients[100];

int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *handle_client (void *arg);

void broadcast_message (int sender_fd, const char *message, size_t len);
int
main ()
{
  int server_sock_fd = socket (AF_INET6, SOCK_STREAM, 0);
  if (server_sock_fd == -1)
    {
      perror ("file descriptor somehow wasnt created, girl\n");
      exit (EXIT_FAILURE);
    }
  int opt_free_port = 1;
  setsockopt (server_sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt_free_port,
              sizeof (opt_free_port));

  struct sockaddr_in6 addr = { 0 };
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons (port);
  addr.sin6_addr = in6addr_any;

  if (bind (server_sock_fd, (const struct sockaddr *)&addr, sizeof (addr))
      == -1)
    {

      perror ("bind() didnt workout");
      close (server_sock_fd);
      exit (EXIT_FAILURE);
    }
  if (listen (server_sock_fd, 11) == -1)
    {
      perror ("listen() didnt workout");
      close (server_sock_fd);
      exit (EXIT_FAILURE);
    }
  printf ("Server's port is %d\n", port);

  while (1)
    {
      int *client_sock_fd = malloc (sizeof (int));
      *client_sock_fd = accept (server_sock_fd, NULL, NULL);

      if (*client_sock_fd < 0)
        {
          perror ("Accept failed");
          free (client_sock_fd);
          continue;
        }

      pthread_t thread;
      if (pthread_create (&thread, NULL, handle_client, client_sock_fd) != 0)
        {
          perror ("Thread creation failed");
          close (*client_sock_fd);
          free (client_sock_fd);
        }
      pthread_detach (thread);
    }

  close (server_sock_fd);
  return 0;
}

void
broadcast_message (int sender_fd, const char *message, size_t len)
{
  pthread_mutex_lock (&clients_mutex);
  for (int i = 0; i < client_count; i++)
    {
      if (clients[i].sockfd != sender_fd)
        {
          send (clients[i].sockfd, message, len, 0);
        }
    }
  pthread_mutex_unlock (&clients_mutex);
}

void *
handle_client (void *arg)
{
  int client_sock_fd = *((int *)arg);
  free (arg);

  pthread_mutex_lock (&clients_mutex);
  clients[client_count].sockfd = client_sock_fd;
  client_count++;
  pthread_mutex_unlock (&clients_mutex);

  char buffer[1024];
  printf ("New client connected (total: %d)\n", client_count);

  while (1)
    {
      ssize_t rec_bytes = recv (client_sock_fd, buffer, sizeof (buffer), 0);
      if (rec_bytes <= 0)
        {
          printf ("Client disconnected\n");
          break;
        }

      buffer[rec_bytes] = '\0';
      printf ("Received: %s", buffer);

      broadcast_message (client_sock_fd, buffer, rec_bytes);
    }

  pthread_mutex_lock (&clients_mutex);
  for (int i = 0; i < client_count; i++)
    {
      if (clients[i].sockfd == client_sock_fd)
        {
          clients[i] = clients[client_count - 1];
          client_count--;
          break;
        }
    }
  pthread_mutex_unlock (&clients_mutex);

  close (client_sock_fd);
  return NULL;
}