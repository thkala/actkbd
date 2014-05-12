/*
 * actkbd - A keyboard shortcut daemon
 *
 * Copyright (c) 2005 Theodoros V. Kalamatianos <nyb@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "actkbd.h"

#include <linux/input.h>


#define PROCFS "/proc/"
#define HANDLERS "bus/input/handlers"
#define DEVICES "bus/input/devices"
#define DEVNODE "/dev/input/event"


/* The device node */
static char devnode[32];

/* The device file pointer */
static FILE *dev;


int init_dev() {
    FILE *fp = NULL;
    int ret;
    unsigned int u0, u1;

    maxkey = KEY_MAX;

    fp = fopen(PROCFS HANDLERS, "r");
    if (fp == NULL) {
	lprintf("Error: could not open " PROCFS HANDLERS ": %s\n", strerror(errno));
	return HOSTFAIL;
    }
    do {
	ret = fscanf(fp, "N: Number=%u Name=evdev Minor=%u", &u0, &u1);
	fscanf(fp, "%*s\n");
    } while ((!feof(fp)) && (ret < 2));
    if (ret < 2) {
	lprintf("Error: event interface not available\n");
	return HOSTFAIL;
    }
    if (verbose > 1)
	lprintf("Event interface present (handler %u)\n", u0);
    fclose(fp);

    /* Skip auto-detection when the device has been specified */
    if (device)
	return OK;

    fp = fopen(PROCFS DEVICES, "r");
    if (fp == NULL) {
	lprintf("Error: could not open " PROCFS DEVICES ": %s\n", strerror(errno));
	return HOSTFAIL;
    }
    do {
	ret = fscanf(fp, "H: Handlers=kbd event%u", &u0);
	fscanf(fp, "%*s\n");
    } while ((!feof(fp)) && (ret < 1));
    if (ret < 1) {
	lprintf("Error: could not detect a usable keyboard device\n");
	return HOSTFAIL;
    }
    if (verbose > 1)
	lprintf("Detected a usable keyboard device (event%u)\n", u0);
    fclose(fp);
    sprintf(devnode, DEVNODE "%u", u0);
    device = devnode;

    return OK;
}


int open_dev() {
    dev = fopen(device, "r");
    if (dev == NULL) {
	lprintf("Error: could not open %s: %s\n", device, strerror(errno));
	return DEVFAIL;
    }
    return OK;
}


int close_dev() {
    fclose(dev);
    return OK;
}


int get_key(int *key, int *type) {
    struct input_event ev;
    int ret;

    do {
	ret = fread (&ev, sizeof (struct input_event), 1, dev);
	if (ret < 1) {
	    lprintf("Error: failed to read event from %s: %s", device, strerror(errno));
	    return READERR;
	}
    } while (ev.type != EV_KEY);

    *key = ev.code;

    switch (ev.value) {
	case 0:
	    *type = REL;
	    break;
	case 1:
	    *type = KEY;
	    break;
	case 2:
	    *type = REP;
	    break;
	default:
	    *type = INVALID;
    }

    if (*key > KEY_MAX)
	*type = INVALID;

    if (*type == INVALID) {
        lprintf("Error: invalid event read from %s: code = %u, value = %u", device, ev.code, ev.value);
	return EVERR;
    }

    return OK;
}
