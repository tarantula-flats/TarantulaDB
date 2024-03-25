CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGET=../../bin/tarantuladb-server

all: $(TARGET)

$(TARGET): tarantuladb-server.c
	$(CC) $(CFLAGS) tarantuladb-server.c -o $(TARGET)

clean:
	rm -f $(TARGET)
