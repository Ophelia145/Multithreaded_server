#include "server.h"
int
main ()
{
  int server_fd = socket (AF_INET6, SOCK_STREAM, 0);
  if (server_fd < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  int opt = 1;
  setsockopt (server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  struct sockaddr_in6 addr = { 0 };
  addr.sin6_family = AF_INET6;
  addr.sin6_port = htons (PORT);
  addr.sin6_addr = in6addr_any;

  if (bind (server_fd, (struct sockaddr *)&addr, sizeof (addr)))
    {
      perror ("bind");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  if (listen (server_fd, 10))
    {
      perror ("listen");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  printf ("Server listening on port %d\n", PORT);

  while (1)
    {
      struct sockaddr_in6 client_addr;
      socklen_t addr_len = sizeof (client_addr);
      int *client_fd = malloc (sizeof (int));

      *client_fd
          = accept (server_fd, (struct sockaddr *)&client_addr, &addr_len);
      if (*client_fd < 0)
        {
          perror ("accept");
          free (client_fd);
          continue;
        }

      pthread_t thread;
      if (pthread_create (&thread, NULL, handle_client, client_fd))
        {
          perror ("pthread_create");
          close (*client_fd);
          free (client_fd);
        }
    }

  close (server_fd);
  return 0;
}

void *
handle_client (void *arg)
{
  int client_fd = *((int *)arg);
  free (arg);
  char buffer[1024];
  ClientInfo client_info = { 0 };
  client_info.sockfd = client_fd;
  client_info.authenticated = 0;

  ssize_t len = recv (client_fd, buffer, sizeof (buffer), 0);
  if (len <= 0)
    {
      close (client_fd);
      return NULL;
    }
  buffer[len] = '\0';

  if (sscanf (buffer, "REGISTER %31s %46s", client_info.username,
              client_info.ip)
      != 2)
    {
      const char *err = "Invalid registration format\n";
      send (client_fd, err, strlen (err), 0);
      close (client_fd);
      return NULL;
    }

  pthread_mutex_lock (&clients_mutex);
  for (int i = 0; i < client_count; i++)
    {
      if (strcmp (clients[i].username, client_info.username) == 0)
        {
          const char *err = "Username already taken\n";
          send (client_fd, err, strlen (err), 0);
          close (client_fd);
          pthread_mutex_unlock (&clients_mutex);
          return NULL;
        }
    }

  if (client_count < MAX_CLIENTS)
    {
      clients[client_count++] = client_info;
      printf ("New client: %s [%s]\n", client_info.username, client_info.ip);
    }
  else
    {
      const char *err = "Server full\n";
      send (client_fd, err, strlen (err), 0);
      close (client_fd);
      pthread_mutex_unlock (&clients_mutex);
      return NULL;
    }
  pthread_mutex_unlock (&clients_mutex);

  while ((len = recv (client_fd, buffer, sizeof (buffer), 0)) > 0)
    {
      buffer[len] = '\0';
      broadcast_message (client_fd, buffer, len);
    }

  pthread_mutex_lock (&clients_mutex);
  for (int i = 0; i < client_count; i++)
    {
      if (clients[i].sockfd == client_fd)
        {
          clients[i] = clients[--client_count];
          break;
        }
    }
  pthread_mutex_unlock (&clients_mutex);

  close (client_fd);
  return NULL;
}

void
broadcast_message (int sender_fd, const char *message, size_t len)
{
  pthread_mutex_lock (&clients_mutex);

  char sender_name[32] = "Unknown";
  for (int i = 0; i < client_count; i++)
    {
      if (clients[i].sockfd == sender_fd)
        {
          strncpy (sender_name, clients[i].username, sizeof (sender_name));
          break;
        }
    }

  char formatted_msg[1024];
  snprintf (formatted_msg, sizeof (formatted_msg), "[%s] %.*s", sender_name,
            (int)len, message);

  for (int i = 0; i < client_count; i++)
    {
      if (clients[i].sockfd != sender_fd)
        {
          send (clients[i].sockfd, formatted_msg, strlen (formatted_msg), 0);
        }
    }
  pthread_mutex_unlock (&clients_mutex);
}
