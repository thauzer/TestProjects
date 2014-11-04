CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

PRINT_SRCS=doxymain.c test.c
PRINT_OBJS=$(PRINT_SRCS:.c=.o)
PRINT_EXEC=doxytest

all: $(PRINT_EXEC)

#server
$(PRINT_EXEC): $(PRINT_OBJS)
	$(CC) $(LDFLAGS) $(PRINT_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(PRINT_EXEC)
