
CC=gcc

CFLAGS=-O3 -Wall -g -pg

dfserve : LIBS+=-lcrypto

all : dfserve pulldf pushdf

dfserve : dfserve.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

pulldf : pulldf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)

pushdf : pushdf.o
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) $(LIBS)
