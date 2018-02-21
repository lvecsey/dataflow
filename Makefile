
CC=gcc

CFLAGS=-O3 -Wall -g -pg

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



