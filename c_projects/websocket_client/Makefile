CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=-lm 

CLIENT_SRCS=client.c cJSON.c command_handler.c
CLIENT_OBJS=$(CLIENT_SRCS:.c=.o)
CLIENT_EXEC=client

all: $(CLIENT_EXEC)

# client
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(SRV_EXEC) $(CLIENT_EXEC)
