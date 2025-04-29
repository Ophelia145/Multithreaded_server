CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11

CLIENT_SRC = client.c
SERVER_SRC = server.c

CLIENT_BIN = client
SERVER_BIN = server

.PHONY: all clean

all: $(CLIENT_BIN) $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRC)
	
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_BIN): $(SERVER_SRC)
	
	$(CC) $(CFLAGS) -o $@ $^

rebuild: clean all

clean:
	@rm -rf $(CLIENT_BIN) $(SERVER_BIN)
