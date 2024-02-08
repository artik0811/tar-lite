.PHONY: all clean

CC = gcc
SOURCES = tar-lite.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = tar-lite
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE)

$(OBJECTS): $(SOURCES)
	$(CC) -c $(SOURCES)

clean:
	rm -rf *.o
