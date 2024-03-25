CC=gcc
CFLAGS=-Wall -Wextra -O2
TARGET=../../bin/tarantuladb-client

all: $(TARGET)

$(TARGET): tarantuladb-client.c
	$(CC) $(CFLAGS) tarantuladb-client.c -o $(TARGET)

clean:
	rm -f $(TARGET)
