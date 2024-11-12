CC=gcc
CFLAGS=-O2 -g3
SRCS=$(wildcard *.c) $(wildcard default.website/cgi-bin/*.c)
PROGS=$(patsubst %.c,%,$(SRCS))
LIBS=-lm -lcrypto -lsqlite3

all: $(PROGS)
clean:
	rm -f $(PROGS)
host: $(PROGS)
	if [ ! -f shapow.db ] ; then ./shapow-mkdb ; fi
	./althttpd -root $(shell pwd) -port 8080

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
	chmod 755 $@