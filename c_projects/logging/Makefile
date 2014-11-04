CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

PRINT_SRCS=main.c
PRINT_OBJS=$(PRINT_SRCS:.c=.o)
PRINT_EXEC=logtest

all: $(PRINT_EXEC)

$(PRINT_EXEC): $(PRINT_OBJS)
	$(CC) $(LDFLAGS) $(PRINT_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(PRINT_EXEC)
