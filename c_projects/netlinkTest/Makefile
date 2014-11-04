CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

NTTEST_SRCS=netlinkTest.c
NTTEST_OBJS=$(NTTEST_SRCS:.c=.o)
NTTEST_EXEC=netlinkTest

all: $(NTTEST_EXEC)

#server
$(NTTEST_EXEC): $(NTTEST_OBJS)
	$(CC) $(LDFLAGS) $(NTTEST_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(NTTEST_EXEC)
