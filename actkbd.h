/*
 * actkbd - A keyboard shortcut daemon
 *
 * Copyright (c) 2005-2006 Theodoros V. Kalamatianos <nyb@users.sourceforge.net>
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
#include <poll.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>


#define UNUSED 0

/* Convert macro name into string */
#define MACSTR(tok)	#tok

/* Event types */
#define INVALID		0
#define KEY		(1<<0)
#define REP		(1<<1)
#define REL		(1<<2)


/* Return values */
enum { OK, USAGE, MEMERR, HOSTFAIL, DEVFAIL, READERR, WRITEERR, EVERR, CONFERR,
	FORKERR, INTERR, PIDERR, NOMATCH };


/* Verbosity level */
extern int verbose;

/* Maximum number of keys */
extern int maxkey;

/* Unix util mode */
extern int showkey;

/* Device grab state */
extern int grabbed;

/* The device name */
extern char *device;

/* The configuration file name */
extern char *config;


/* Logging function */
int lprintf(const char *fmt, ...);

/* Key printing function */
void printkeys(struct timeval time, int key, int value);

/* Formatted string concatenation function */
void strfcat(char *src, char *fmt, ...);

/* Device initialisation */
int init_dev();

/* Device open function */
int open_dev();

/* Device close function */
int close_dev();

/* Device grab function */
int grab_dev();

/* Device un-grab function */
int ungrab_dev();

/* Keyboard event receiver function */
int get_key(int *key, int *type, int *value, struct timeval *time, int tickrate, int poll, int num);

/* Send an event to the input layer */
int snd_key(int key, int type);

/* Set a keyboard LED */
int set_led(int led, int on);

/* Convert keycode value, to a string with the keycodes name. */
extern const char *keycodetostr(int type, int code, int numeric);

/* Key mask handling */
int get_masksize();
int init_mask(unsigned char **mask);
void free_mask(unsigned char **mask);
int lprint_mask(unsigned char *mask);
int strmask(unsigned char **mask, char *keys);

/* The active key mask */
int init_key_mask();
void free_key_mask();
void clear_key_mask();
int set_key_bit(int bit, int val);
int get_key_bit(int bit);
int cmp_key_mask(unsigned char *mask0, unsigned int attr);
int lprint_key_mask_delim();
int sprint_key_mask_delim(unsigned char *c, char d, char *str, int numeric);
int lprint_key_mask();
int print_key_mask(char *str, int numeric);
unsigned char *get_key_mask();

/* The ignored key mask */
int init_ign_mask();
void free_ign_mask();
void clear_ign_mask();
int set_ign_bit(int bit, int val);
int get_ign_bit(int bit);
int cmp_ign_mask(unsigned char *mask0, unsigned int attr);
int lprint_ign_mask_delim(char c);
int lprint_ign_mask();
unsigned char *get_ign_mask();
void copy_key_to_ign_mask();


/* The attribute node struct */
typedef struct _attr_t attr_t;
struct _attr_t {
    int type;			/* Attribute type */
    void *opt;			/* Options for this attribute */
    attr_t *next;		/* The next node */
};

/* Supported attributes */
#define ATTR_EXEC		0
#define ATTR_GRAB		1
#define ATTR_UNGRAB		2
#define ATTR_IGNREL		3
#define ATTR_RCVREL		4
#define ATTR_ALLREL		5
#define ATTR_KEY		6
#define ATTR_REL		7
#define ATTR_REP		8
#define ATTR_LEDON		9
#define ATTR_LEDOFF		10
#define ATTR_SET		11
#define ATTR_UNSET		12


/* The key_cmd struct */
typedef struct {
    unsigned char *keys;	/* The key mask */
    int type;			/* The event type */
    char *command;		/* The command to execute */

    unsigned int attr_bits;	/* Bitwise attributes */

    attr_t *attrs;		/* The attribute list */
} key_cmd;

/* The bitwise attribute values */
#define BIT_ATTR_NOEXEC		(1<<0)	/* Do not call system() */
#define BIT_ATTR_GRABBED	(1<<1)	/* Match only when the device is grabbed */
#define BIT_ATTR_UNGRABBED	(1<<2)	/* Match only when the device is not grabbed */
#define BIT_ATTR_NOT		(1<<3)	/* Match any key except for the specified ones */
#define BIT_ATTR_ALL		(1<<4)	/* Match if all of the specified keys is pressed */
#define BIT_ATTR_ANY		(1<<5)	/* Match if any of the specified keys is pressed */


/* Configuration file processing */
int open_config();
int close_config();
int match_key(int type, key_cmd **command);


#endif /* _ACTKBD_H_ */
