CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=endianswap.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=endianswap

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(EXECUTABLE) *.o
