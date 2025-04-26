#include <arpa/inet.h>
#include <netinet/ip6.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void *send_thread (void *arg);
int
main ()
{

  int client_sock_fd = socket (AF_INET6, SOCK_STREAM, 0);
  if (client_sock_fd < 0)
    {
      perror ("fd wasn't created, so socket wasnt as well");
      exit (EXIT_FAILURE);
    }

  struct sockaddr_in6 serv_addr = { 0 };
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_port = htons (8080);

  if (inet_pton (AF_INET6, "::1", &serv_addr.sin6_addr) != 1)
    {
      perror ("inet_pton is a failure");
      close (client_sock_fd);
      exit (EXIT_FAILURE);
    }
  if (connect (client_sock_fd, (const struct sockaddr *)&serv_addr,
               sizeof (serv_addr))
      == -1)
    {
      perror ("connect() didn work out");
      close (client_sock_fd);
      exit (EXIT_FAILURE);
    }
  printf ("Connected to server. Type:\n");
  pthread_t thread;
  if (pthread_create (&thread, NULL, send_thread, &client_sock_fd) != 0)
    {
      perror ("pthread_create failed");
      close (client_sock_fd);
      exit (EXIT_FAILURE);
    }
  char buffer[1024];
  while (1)
    {
      ssize_t bytes = recv (client_sock_fd, buffer, sizeof (buffer) - 1, 0);
      if (bytes <= 0)
        {
          printf ("server disconnected\n");
          break;
        }
      buffer[bytes] = '\0';
      printf ("server replies with: %s", buffer);
    }
  close (client_sock_fd);

  return 0;
}

void *
send_thread (void *arg)
{
  int sockfd = *(int *)arg;
  char buffer[1024];

  while (1)
    {
      fgets (buffer, sizeof (buffer), stdin);
      if (send (sockfd, buffer, strlen (buffer), 0) < 0)
        {
          perror ("send failed");
          break;
        }
    }
  return NULL;
}
