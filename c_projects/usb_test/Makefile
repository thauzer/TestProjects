CC=gcc

CFLAGS_ALL=-c -Wall
#CFLAGS_ERROR=-Werror
CFLAGS_INCLUDE=-I. -Iinclude
CFLAGS=$(CFLAGS_ALL) $(CFLAGS_ERROR) $(CFLAGS_INCLUDE)
LDFLAGS=-lusb-1.0 #-Llib

USB_SRCS=usbComm.c
USB_OBJS=$(USB_SRCS:.c=.o)
USB_EXEC=usbComm

all: $(USB_EXEC)

#server
$(USB_EXEC): $(USB_OBJS)
	$(CC) $(LDFLAGS) $(USB_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(USB_EXEC)
