CC=g++

CFLAGS=-c -g -Wall -Werror -I.
LDFLAGS= -g

SRCS=dfdeque.c
OBJS=$(SRCS:.c=.o)
EXEC=df_deque

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(EXEC)
