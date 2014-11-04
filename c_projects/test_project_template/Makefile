CC=gcc

CFLAGS=-c -Wall -I.
LDFLAGS=

APP_SRCS=main.c
APP_OBJS=$(APP_SRCS:.c=.o)
APP_EXEC=clipTest

all: $(APP_EXEC)

#function pointers example
$(APP_EXEC): $(APP_OBJS)
	$(CC) $(LDFLAGS) $(APP_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(APP_EXEC)
