CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

LIBTEST_SRCS=printtest.c
LIBTEST_OBJS=$(LIBTEST_SRCS:.c=.o)
LIBTEST_EXEC=libtest

all: $(PRINT_EXEC)

#server
$(PRINT_EXEC): $(LIBTEST_OBJS)
	$(CC) $(LDFLAGS) $(LIBTEST_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(LIBTEST_EXEC)
