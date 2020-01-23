GCC = /root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc
# gcc
COPTS = -Wall
CLIENT = client
BIN = client

all:
	$(GCC) $(COPTS) lib/comm.c $(CLIENT).c -ljpeg -o $(BIN)

clean:
	rm -f $(BIN)

# usr/local/lib
