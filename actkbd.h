/*
 * actkbd - A keyboard shortcut daemon
 *
 * Copyright (c) 2005 Theodoros V. Kalamatianos <nyb@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef _ACTKBD_H_
#define _ACTKBD_H_

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <getopt.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>

#include "version.h"

/* Event types */
enum { KEY, REP, REL, INVALID };

/* Return values */
enum { OK, USAGE, MEMERR, HOSTFAIL, DEVFAIL, READERR, EVERR, CONFERR, FORKERR, INTERR, PIDERR, NOMATCH };

/* Verbosity level */
extern int verbose;

/* Maximum number of keys */
extern int maxkey;

/* The device name */
extern char *device;

/* The configuration file name */
extern char *config;

/* Logging function */
int lprintf(const char *fmt, ...);

/* Device initialisation */
int init_dev();

/* Device open function */
int open_dev();

/* Device close function */
int close_dev();

/* Keyboard event receiver function */
int get_key(int *key, int *type);

/* Key mask handling */
int get_masksize();
int init_mask(unsigned char **mask);
int clear_mask(unsigned char **mask);
int cmp_mask(unsigned char *mask0, unsigned char* mask1);
int set_bit(unsigned char *mask, int bit, int val);
int get_bit(unsigned char *mask, int bit);
int lprint_mask_delim(unsigned char *mask, char d);
int lprint_mask(unsigned char *mask);
int init_key_mask();
int clear_key_mask();
int set_key_bit(int bit, int val);
int get_key_bit(int bit);
int cmp_key_mask(unsigned char *mask0);
int lprint_key_mask_delim(char c);
int lprint_key_mask();
unsigned char *get_key_mask();

/* Configuration file processing */
int open_config();
int close_config();
int get_command(unsigned char *mask, int type, char **command);

/* The key_cmd struct */
typedef struct {
    unsigned char *keys;
    int type;
    char *module;
    char *command;
} key_cmd;

#endif /* _ACTKBD_H_ */
