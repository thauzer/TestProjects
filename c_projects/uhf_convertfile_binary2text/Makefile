CC=gcc

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

CONVB2T_SRCS=mainConvb2t.c
CONVB2T_OBJS=$(CONVB2T_SRCS:.c=.o)
CONVB2T_EXEC=convb2t

all: $(CONVB2T_EXEC)

$(CONVB2T_EXEC): $(CONVB2T_OBJS)
	$(CC) $(LDFLAGS) $(CONVB2T_OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(CONVB2T_EXEC)
