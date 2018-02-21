
CC=gcc

CFLAGS=-O3 -Wall -g -pg

MACOS_CFLAGS = $(CFLAGS) -I/usr/local/Cellar/openssl/1.0.2n/include
MACOS_LIBS = -L/usr/local/Cellar/openssl/1.0.2n/lib

dfserve : LIBS+=-lcrypto

all : dfserve pulldf pushdf genrep genbindlist showrep

dfserve : dfserve.o dataflow.o rephp.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

pulldf : pulldf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

pushdf : pushdf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

genrep : genrep.o hostport.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

genbindlist : genbindlist.o hostport.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

showrep : showrep.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)



