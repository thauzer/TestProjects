EVT_SRC = evtmain.c
EVT_OBJS = evtmain.o

SEND_SRC = sendmain.c
SEND_OBJS = sendmain.o

CC = gcc

all: evt send

evt: $(EVT_OBJS)
	$(CC) -o $@ $(EVT_OBJS)

send: $(SEND_OBJS)
	$(CC) -o $@ $(SEND_OBJS)

clean:
	rm -f *.o evt send
