CC=g++

CFLAGS=-c -Wall -Werror -I.
LDFLAGS=

TEMPS_SRCS=templates.cpp
TEMPS_OBJS=$(TEMPS_SRCS:.cpp=.o)
TEMPS_EXEC=templates

all: $(TEMPS_EXEC)

#server
$(TEMPS_EXEC): $(TEMPS_OBJS)
	$(CC) $(LDFLAGS) $(TEMPS_OBJS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o $(TEMPS_EXEC)
