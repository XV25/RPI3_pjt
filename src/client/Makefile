GCC = gcc
COPTS = -Wall
CLIENT = client
BIN = client

all:
	$(GCC) $(COPTS) lib/comm.c $(CLIENT).c -o $(BIN)

clean:
	rm -f $(BIN)