/*
 * actkbd - A keyboard shortcut daemon
 *
 * Copyright (c) 2005-2006 Theodoros V. Kalamatianos <nyb@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "actkbd.h"

#include <regex.h>
#include <sys/ioctl.h>

#include <linux/input.h>

#define test_bit(bit, array) (array[bit / 8] & (1 << (bit % 8)))
#define PROCFS "/proc/"
#define HANDLERS "bus/input/handlers"
#define DEVICES "bus/input/devices"
#define DEVNODE "/dev/input/event"


/* The device node */
static char devnode[32];

/* The device file pointer */
static FILE *dev;

/* The device file decriptor */
static int devfd;


int init_dev() {
    FILE *fp = NULL;
    int ret = -1;
    unsigned int u0 = 0, u1 = 0;
    regex_t preg;
    regmatch_t pmatch[4];
    /*for (int r; r<4; r++) {
	    pmatch[r].rm_so=0;
	    pmatch[r].rm_eo=0;
    }*/

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

    /* Compile the regular expression and scan for it */
    ret = -1;
    regcomp(&preg, "^H: Handlers=(.* )?kbd (.* )?event([0-9]+)? leds", REG_EXTENDED);
    do {
	char l[128] = "";
	void *str = fgets(l, 128, fp);
	if (str == NULL)
	    break;
	ret = regexec(&preg, l, 4, pmatch, 0);
	/*printf("ret: %i\nu0: %u\n", ret, u0);
	printf(
	    "pmatch[0].rm_so: %li, pmatch[0].rm_eo: %li\n"
	    "pmatch[1].rm_so: %li, pmatch[1].rm_eo: %li\n"
	    "pmatch[2].rm_so: %li, pmatch[2].rm_eo: %li\n"
	    "pmatch[3].rm_so: %li, pmatch[3].rm_eo: %li\n"
	    "pmatch[4].rm_so: %li, pmatch[4].rm_eo: %li\n"
	    "pmatch[5].rm_so: %li, pmatch[5].rm_eo: %li\n"
	    "pmatch[6].rm_so: %li, pmatch[6].rm_eo: %li\n"
	    "pmatch[7].rm_so: %li, pmatch[7].rm_eo: %li\n"
	    , pmatch[0].rm_so, pmatch[0].rm_eo
            , pmatch[1].rm_so, pmatch[1].rm_eo
            , pmatch[2].rm_so, pmatch[2].rm_eo
            , pmatch[3].rm_so, pmatch[3].rm_eo);
	    , pmatch[4].rm_so, pmatch[4].rm_eo
	    , pmatch[5].rm_so, pmatch[5].rm_eo
	    , pmatch[6].rm_so, pmatch[6].rm_eo
	    , pmatch[7].rm_so, pmatch[7].rm_eo);*/
	if (ret == 0) {
	    l[pmatch[3].rm_eo] = '\0';
	    ret = sscanf((l+pmatch[3].rm_so), "%u", &u0);
	    /*printf("ret: %i\nu0: %u\nl+pmatch[3].rm_so: %s\n", ret, u0, l+pmatch[3].rm_so);
	    printf(
	        "pmatch[0].rm_so: %li, pmatch[0].rm_eo: %li\n"
	        "pmatch[1].rm_so: %li, pmatch[1].rm_eo: %li\n"
	        "pmatch[2].rm_so: %li, pmatch[2].rm_eo: %li\n"
	        "pmatch[3].rm_so: %li, pmatch[3].rm_eo: %li\n"
	        "pmatch[4].rm_so: %li, pmatch[4].rm_eo: %li\n"
	        "pmatch[5].rm_so: %li, pmatch[5].rm_eo: %li\n"
	        "pmatch[6].rm_so: %li, pmatch[6].rm_eo: %li\n"
	        "pmatch[7].rm_so: %li, pmatch[7].rm_eo: %li\n"
		, pmatch[0].rm_so, pmatch[0].rm_eo
                , pmatch[1].rm_so, pmatch[1].rm_eo
                , pmatch[2].rm_so, pmatch[2].rm_eo
                , pmatch[3].rm_so, pmatch[3].rm_eo);
	        , pmatch[4].rm_so, pmatch[4].rm_eo
	        , pmatch[5].rm_so, pmatch[5].rm_eo
	        , pmatch[6].rm_so, pmatch[6].rm_eo
	        , pmatch[7].rm_so, pmatch[7].rm_eo);*/
	} else {
	    ret = -1;
	}
    } while ((!feof(fp)) && (ret < 1));
    regfree(&preg);

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
    dev = fopen(device, "a+");
    devfd = open(device, O_RDONLY);
    if (dev == NULL || devfd == -1) {
	lprintf("Error: could not open %s: %s\n", device, strerror(errno));
	return DEVFAIL;
    }
    return OK;
}


int close_dev() {
    fclose(dev);
    close(devfd);
    return OK;
}


int grab_dev() {
    int ret;

    if (grabbed)
	return 0;

    ret = ioctl(fileno(dev), EVIOCGRAB, (void *)1);
    if (ret == 0)
	grabbed = 1;
    else
	lprintf("Error: could not grab %s: %s\n", device, strerror(errno));

    return ret;
}


int ungrab_dev() {
    int ret;

    if (!grabbed)
	return 0;

    ret = ioctl(fileno(dev), EVIOCGRAB, (void *)0);
    if (ret == 0)
	grabbed = 0;
    else
	lprintf("Error: could not ungrab %s: %s\n", device, strerror(errno));

    return ret;
}


int get_key(int *key, int *type, int *value, struct timeval *time, int tickrate, int poll, int num) {
    struct input_event ev;
    int ret, ticks = 0, keydown = 0;
    /* Poll for keypresses */
    if(poll) {
	if (!ticks)
	    printf("Tickrate: %i\n", tickrate);
	while (1) {
	    printf("%i ", ticks++);
	    u_int8_t key_b[KEY_MAX/8 + 1];
	    memset(key_b, 0, sizeof(key_b));
	    ioctl(devfd, EVIOCGKEY(sizeof(key_b)), key_b);
	    int keycount = 0;
	    for (int yalv = 0; yalv < KEY_MAX; yalv++) {
	        if (test_bit(yalv, key_b)) {
	            ++keycount;
	            /* the bit is set in the key state */
	            if (keycount > 1)
	        	(!num) ? printf("+%s", keycodetostr(EV_KEY, yalv, 0)) : printf("+%i", yalv);
	            else if (keycount == 1) {
	        	(!num) ? printf("%s", keycodetostr(EV_KEY, yalv, 0)) : printf("%i", yalv);
	            }
	        }
		keydown = keycount ? 1 : 0;
	    }
	    if (!keydown)
		printf("%s %i", (!num) ? "KEY_NONE" : "0", keydown);
	    else
		printf(" %i", keydown);
	    printf("\n");
	    fflush(stdout);
	    float tickdely = (float)1/tickrate;
	    usleep((useconds_t)(tickdely*1000000));
	}
    /* Wait for an input event */
    } else {
        do {
	    ret = fread(&ev, sizeof(ev), 1, dev);
	    if (ret < 1) {
		lprintf("Error: failed to read event from %s: %s", device, strerror(errno));
		return READERR;
	    }
	} while (ev.type != EV_KEY);
	*key = ev.code;
	*time = ev.time;
	*value = ev.value;
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
		*type = KEY;
	}

	if (*key > KEY_MAX)
	    *type = INVALID;

	if (*type == INVALID) {
            lprintf("Error: invalid event read from %s: code = %u, value = %u", device, ev.code, ev.value);
	    return EVERR;
	}
    }
    return OK;
}


int snd_key(int key, int type) {
    struct input_event ev;
    int ret;

    ev.type = EV_KEY;
    ev.code = key;

    switch (type) {
	case KEY:
	    ev.value = 1;
	    break;
	case REL:
	    ev.value = 0;
	    break;
	case REP:
	    ev.value = 2;
	    break;
	default:
	    return EINVAL;
    }

    ret = fwrite(&ev, sizeof(ev), 1, dev);
    if (ret < 1) {
	lprintf("Error: failed to send event to %s: %s", device, strerror(errno));
	return WRITEERR;
    }

    return OK;
}


int set_led(int led, int on) {
    struct input_event ev;
    int ret;

    ev.type = EV_LED;
    ev.code = led;
    ev.value = (on > 0);

    ret = fwrite(&ev, sizeof(ev), 1, dev);
    if (ret < 1) {
	lprintf("Error: failed to set LED at %s: %s", device, strerror(errno));
	return WRITEERR;
    }

    return OK;
}
