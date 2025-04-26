#include "client.h"

int
main ()
{
  char username[32], ip[INET6_ADDRSTRLEN] = "::1";
  int file_port = find_free_port ();

  printf ("Username: ");
  fgets (username, sizeof (username), stdin);
  username[strcspn (username, "\n")] = '\0';

  int sockfd = socket (AF_INET6, SOCK_STREAM, 0);
  struct sockaddr_in6 addr = { 0 };
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons (8080);
  inet_pton (AF_INET6, "::1", &addr.sin6_addr);

  if (connect (sockfd, (struct sockaddr *)&addr, sizeof (addr)))
    {
      perror ("connect");
      exit (EXIT_FAILURE);
    }

  char reg_msg[256];
  snprintf (reg_msg, sizeof (reg_msg), "REGISTER %s %s %d", username, ip,
            file_port);
  send (sockfd, reg_msg, strlen (reg_msg), 0);

  ClientState state = { file_port, sockfd };
  pthread_t file_thread;
  pthread_create (&file_thread, NULL, file_server, &state);
  pthread_detach (file_thread);

  pthread_t recv_th;
  pthread_create (&recv_th, NULL, recv_thread, &state);

  char buf[1024];
  while (fgets (buf, sizeof (buf), stdin))
    {
      send (sockfd, buf, strlen (buf), 0);
    }

  close (sockfd);
  return 0;
}

int
find_free_port ()
{
  int sock = socket (AF_INET6, SOCK_STREAM, 0);
  struct sockaddr_in6 addr = { 0 };
  addr.sin6_family = AF_INET6;
  addr.sin6_addr = in6addr_any;

  for (int port = FILE_PORT_MIN; port <= FILE_PORT_MAX; port++)
    {
      addr.sin6_port = htons (port);
      if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)) == 0)
        {
          close (sock);
          return port;
        }
    }
  close (sock);
  return -1;
}

void *
file_server (void *arg)
{
  ClientState *state = (ClientState *)arg;
  int sock = socket (AF_INET6, SOCK_STREAM, 0);

  struct sockaddr_in6 addr = { 0 };
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons (state->file_port);
  addr.sin6_addr = in6addr_any;

  if (bind (sock, (struct sockaddr *)&addr, sizeof (addr)))
    {
      perror ("bind");
      return NULL;
    }

  listen (sock, 5);
  printf ("File server started on port %d\n", state->file_port);

  while (1)
    {
      int client = accept (sock, NULL, NULL);
      close (client);
    }

  close (sock);
  return NULL;
}

void *
recv_thread (void *arg)
{
  ClientState *state = (ClientState *)arg;
  char buf[1024];

  while (1)
    {
      ssize_t len = recv (state->sockfd, buf, sizeof (buf), 0);
      if (len <= 0)
        break;
      buf[len] = '\0';
      printf ("%s\n", buf);
    }
  return NULL;
}
