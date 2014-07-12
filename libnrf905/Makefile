CC=gcc
CFLAGS=-Wall -I../bcm2835-1.36/src
LDFLAGS=../bcm2835-1.36/src/libbcm2835.a

all: libnrf905.so nrf905_recv nrf905_send nrf905_status

libnrf905.so: nrf905.o
	$(CC) -shared -fPIC $(CFLAGS) $< -o $@

nrf905_recv: nrf905_recv.o
	$(CC) $(CFLAGS) $< -o $@ -L. -lnrf905 $(LDFLAGS)

nrf905_send: nrf905_send.o
	$(CC) $(CFLAGS) $< -o $@ -L. -lnrf905 $(LDFLAGS)

nrf905_status: nrf905_status.o
	$(CC) $(CFLAGS) $< -o $@ -L. -lnrf905 $(LDFLAGS)

nrf905.o: nrf905.c nrf905.h
nrf905_send.o: nrf905_send.c nrf905.h
nrf905_recv.o: nrf905_recv.c nrf905.h
nrf905_status.o: nrf905_status.c nrf905.h
