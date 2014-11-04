CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

SRV_SRCS=server.c socketinit.c evt_poll.c
SRV_OBJS=$(SRV_SRCS:.c=.o)
SRV_EXEC=server

CLIENT_SRCS=client.c
CLIENT_OBJS=$(CLIENT_SRCS:.c=.o)
CLIENT_EXEC=client

all: $(SRV_EXEC) $(CLIENT_EXEC)

#server
$(SRV_EXEC): $(SRV_OBJS)
	$(CC) $(LDFLAGS) $(SRV_OBJS) -o $@

# client
$(CLIENT_EXEC): $(CLIENT_OBJS)
	$(CC) $(LDFLAGS) $(CLIENT_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(SRV_EXEC) $(CLIENT_EXEC)
