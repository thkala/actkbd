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


/* Active key mask */
static unsigned char *mask = NULL;

/* Key mask size */
static int masksize = 0;


int get_masksize() {
    return masksize;
}


/* Mask initialisation */
int init_mask(unsigned char **mask) {
    /* Calculate the key mask size */
    masksize = (maxkey + 1) / 8;
    if (((maxkey + 1) % 8) > 0)
	++masksize;

    /* Allocate the key mask space */
    *mask = (unsigned char *)(malloc(masksize));

    memset(*mask, 0, masksize);

    return OK;
}


/* Mask termination */
int clear_mask(unsigned char **mask) {
    free(*mask);
    *mask = NULL;
    return OK;
}


/* Mask comparison */
int cmp_mask(unsigned char *mask0, unsigned char* mask1) {
    return memcmp(mask0, mask1, masksize);
}


/* Set a bit */
int set_bit(unsigned char *mask, int bit, int val) {
    unsigned char byte = 1;

    if ((bit < 0) || (bit > maxkey) || (val < 0) || (val > 1)) {
	if (verbose > 1)
	    lprintf("Error: invalid set_bit arguments %i, %i\n", bit, val);
	return INTERR;
    }

    byte = byte << (bit % 8);

    if (val == 0)
	mask[bit / 8] = mask[bit / 8] & (~byte);
    else
	mask[bit / 8] = mask[bit / 8] | byte;

    return OK;
}


/* Get a bit */
int get_bit(unsigned char *mask, int bit) {
    unsigned char byte = 1;

    if ((bit < 0) || (bit > maxkey)) {
	lprintf("Error: invalid get_bit argument %i\n", bit);
	return -1;
    }

    byte = byte << (bit % 8);
    return ((mask[bit / 8] & byte) > 0);
}


/* Print a key mask */
int lprint_mask_delim(unsigned char *mask, char d) {
    int i = 0, c = 0;

    if (mask == NULL) {
	lprintf("Error: attempt to dereference NULL mask pointer\n");
	return INTERR;
    }

    for (i = 0; i <= maxkey; ++i) {
	if (get_bit(mask, i)) {
	    ++c;
	    if (c > 1)
		lprintf("%c%i", d, i);
	    else
		lprintf("%i", i);
	}
    }
    return OK;
}

int lprint_mask(unsigned char *mask) {
    return lprint_mask_delim(mask, '+');
}


/* The active key mask */
int init_key_mask() {
    return init_mask(&mask);
}

int clear_key_mask() {
    return clear_mask(&mask);
}

int set_key_bit(int bit, int val) {
    return set_bit(mask, bit, val);
}

int get_key_bit(int bit) {
    return get_bit(mask, bit);
}

int cmp_key_mask(unsigned char *mask0) {
    return cmp_mask(mask, mask0);
}

int lprint_key_mask_delim(char d) {
    return lprint_mask_delim(mask, d);
}

int lprint_key_mask() {
    return lprint_mask(mask);
}

unsigned char *get_key_mask() {
    return mask;
}
