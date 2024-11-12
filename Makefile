CC=gcc
CFLAGS=-O2 -g3
SRCS=$(wildcard *.c) $(wildcard default.website/cgi-bin/*.c)
PROGS=$(patsubst %.c,%,$(SRCS))
LIBS=-lm -lcrypto -lsqlite3

STATIC=default.website/index.html default.website/demo.js
STATIC_ZIP=$(patsubst %,%.gz,$(STATIC))

all: $(PROGS)
clean:
	rm -f $(PROGS) $(STATIC_ZIP)
compress:
	for i in $(STATIC) ; do gzip -c9 < $$i > $$i.gz ; done
host: $(PROGS) compress
	if [ ! -f shapow.db ] ; then ./shapow-mkdb ; fi
	./althttpd -root $(shell pwd) -port 8080
.PHONY: all clean host compress

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)
	chmod 755 $@