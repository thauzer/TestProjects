#CC=gcc

#CFLAGS=-c -Wall -I. -g -DSS_UNIX -DGENERIC_INSTALLDB -DMOD_DEBUG
#LDFLAGS=-lm -lpthread -lrt -lodbc

#APP_SRCS=main.c
#APP_OBJS=$(APP_SRCS:.c=.o)
#APP_EXEC=clipTest

#all: $(APP_EXEC)

#function pointers example
#$(APP_EXEC): $(APP_OBJS)
#	$(CC) $(LDFLAGS) $(APP_OBJS) -o $@

#.c.o:
#	$(CC) $(CFLAGS) $< -o $@

#clean:
#	rm -f *.o $(APP_EXEC)

CC=gcc

NETEZZA_CFLAGS= -I. -DSS_UNIX -DGENERIC_INSTALLDB -DMOD_DEBUG
NETEZZA_LDFLAGS=-lm -lpthread -lrt -lodbc

NETEZZA_SRCS=\
	main.c \
	clip_string.c

NETEZZA_EXEC=clipTest

all: $(NETEZZA_EXEC)

$(NETEZZA_EXEC):
	$(CC) $(NETEZZA_CFLAGS) -o $(NETEZZA_EXEC) $(NETEZZA_SRCS) $(NETEZZA_LDFLAGS)

clean:
	rm -f *.o $(NETEZZA_EXEC) 