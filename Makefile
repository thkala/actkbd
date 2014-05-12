#
# actkbd - A keyboard shortcut daemon
#
# Copyright (c) 2005-2006 Theodoros V. Kalamatianos <nyb@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as published by
# the Free Software Foundation.
#

prefix := /usr/local
sbindir := $(prefix)/sbin
sysconfdir := /etc

DEBUG :=
CFLAGS := -O2 -Wall $(DEBUG)



all: actkbd

actkbd: actkbd.o mask.o config.o linux.o

actkbd.o : actkbd.h
mask.o : actkbd.h

config.o : actkbd.h config.c
	$(CC) $(CFLAGS)	-c -o config.o config.c -DCONFIG=\"$(sysconfdir)/actkbd.conf\"

linux.o : actkbd.h

actkbd.h: version.h

install: all
	install -D -m755 actkbd $(sbindir)/actkbd
	install -d -m755 $(sysconfdir)
	echo "# actkbd configuration file" > $(sysconfdir)/actkbd.conf

clean:
	rm -f actkbd *.o
