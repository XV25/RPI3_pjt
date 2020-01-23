GCC = gcc
#/root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc
COPTS = -Wall
CLIENT = client
BIN = client

all:
	$(GCC) $(COPTS) lib/comm.c $(CLIENT).c -o $(BIN)

clean:
	rm -f $(BIN)
