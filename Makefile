prefix := /usr/local
sbindir := $(prefix)/sbin

CFLAGS := -O2 -Wall $(DEBUG)



all: actkbd

actkbd: actkbd.o mask.o config.o linux.o

actkbd.o : actkbd.h
mask.o : actkbd.h
config.o : actkbd.h

linux.o : actkbd.h

actkbd.h: version.h

install: all
	install -D -m755 actkbd $(sbindir)/actkbd

clean:
	rm -f actkbd *.o
