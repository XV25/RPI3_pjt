GCC = /root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc
SERVEUR = serveur1
BIN = serveur1

all:
	$(GCC) lib/comm.c $(SERVEUR).c -o $(BIN)

clean:
	rm -f $(BIN)
