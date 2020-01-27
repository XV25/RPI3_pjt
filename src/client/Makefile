GCC = /root/buildroot-precompiled-2017.08/output/host/usr/bin/arm-linux-gcc
COPTS = -Wall
CLIENT = client
BIN = client


all:

	# $(v4l2grab_OBJECTS) $(v4l2grab_LDADD) $(LIBS)
	$(GCC)  $(COPTS) lib/comm.c $(CLIENT).c -o $(BIN) -ljpeg 

clean:
	rm -f $(BIN)

# usr/local/lib
