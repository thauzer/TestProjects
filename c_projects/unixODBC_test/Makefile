CC=gcc

CFLAGS=-c -Wall -I.
LDFLAGS=-lodbc

APP_SRCS=main.c log.c
APP_OBJS=$(APP_SRCS:.c=.o)
APP_EXEC=odbcTest

all: $(APP_EXEC)

#unixODBC example
$(APP_EXEC): $(APP_OBJS)
	$(CC) $(LDFLAGS) $(APP_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(APP_EXEC)
