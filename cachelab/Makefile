CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c11 -O0 -ggdb -no-pie
LIBS:=-lm
TARGET := cachesim
SOURCES = cachesim.c cache.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: clean all

all: $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ -c

$(TARGET): $(OBJECTS)
		$(CC) $(CFLAGS) $^ -o $@ 
	
clean:
	rm -f *.o $(TARGET) *.d

-include $(OBJECTS:.o=.d)	

