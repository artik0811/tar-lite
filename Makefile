.PHONY: all clean help 

CC = gcc
SOURCES = tar-lite.c
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = tar-lite

help: # show help message
	@grep -E '^[a-zA-Z0-9 -]+:.*#'  Makefile | sort | while read -r l; do printf "\033[1;32m$$(echo $$l | cut -f 1 -d':')\033[00m:$$(echo $$l | cut -f 2- -d'#')\n"; done

all: $(EXECUTABLE) # build 

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE)
	rm $(OBJECTS)

$(OBJECTS): $(SOURCES)
	$(CC) -c $(SOURCES)

clean: # clean *.o files
	rm -rf *.o
