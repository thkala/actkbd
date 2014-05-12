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

/* Ignored key mask */
static unsigned char *ignmask = NULL;

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


/* Mask deallocation */
void free_mask(unsigned char **mask) {
    free(*mask);
    *mask = NULL;
}

/* Mask zeroing */
static void clear_mask(unsigned char **mask) {
    memset(*mask, 0, masksize);
}


/* Mask comparison */
static int cmp_mask(unsigned char *mask0, unsigned char* mask1, unsigned int attr) {
    int i;

    /* Require that at least one mask0 bit is not set in mask1  */
    if ((attr & BIT_ATTR_NOT) != 0) {
	for (i = 0; i < masksize; ++i)
	    if ((mask0[i] & ~mask1[i]) != 0)
		return 1;
	return 0;
    }

    /* Require that all mask1 bits are present in mask0 */
    if ((attr & BIT_ATTR_ALL) != 0) {
	for (i = 0; i < masksize; ++i)
	    if ((mask0[i] & mask1[i]) != mask1[i])
		return 0;
	return 1;
    }

    /* True if any mask1 bits are present in mask0 */
    if ((attr & BIT_ATTR_ANY) != 0) {
	for (i = 0; i < masksize; ++i)
	    if ((mask0[i] & mask1[i]) != 0)
		return 1;
	return 0;
    }

    /* Require an exact match */
    return (memcmp(mask0, mask1, masksize)  == 0);
}


/* Set a bit */
static int set_bit(unsigned char *mask, int bit, int val) {
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
static int get_bit(unsigned char *mask, int bit) {
    unsigned char byte = 1;

    if ((bit < 0) || (bit > maxkey)) {
	lprintf("Error: invalid get_bit argument %i\n", bit);
	return -1;
    }

    byte = byte << (bit % 8);
    return ((mask[bit / 8] & byte) > 0);
}


/* Print a key mask */
static int lprint_mask_delim(unsigned char *mask, char d) {
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


/* Use a A+B+N... string to initialise a key mask */
int strmask(unsigned char **mask, char *keys) {
    int l, i, k;

    /* Default mask pointer */
    *mask = NULL;

    l = strlen(keys);

    /* Clean up the input */
    for (i = 0; i < l; ++i) {
	if ((keys[i] == '+') || (keys[i] == '-') || (keys[i] == ','))
	    keys[i] = ' ';
	if (isspace(keys[i]))
	    keys[i] = '\n';
	if ((!isdigit(keys[i])) && (keys[i] != '\n'))
	    return CONFERR;
    }

    init_mask(mask);

    /* Set the key mask */
    i = 0;
    while (i < l) {
	if (isdigit(keys[i])) {
	    sscanf(keys + i, "%i", &k);
	    if (set_bit(*mask, k, 1) != OK) {
		free(*mask);
		return CONFERR;
	    }
	    while (isdigit(keys[i]))
		++i;
	} else {
	    ++i;
	}
    }

    return OK;
}


/* The active key mask */
int init_key_mask() {
    return init_mask(&mask);
}

void free_key_mask() {
    free_mask(&mask);
}

void clear_key_mask() {
    clear_mask(&mask);
}

int set_key_bit(int bit, int val) {
    return set_bit(mask, bit, val);
}

#if UNUSED
int get_key_bit(int bit) {
    return get_bit(mask, bit);
}
#endif

int cmp_key_mask(unsigned char *mask0, unsigned int attr) {
    return cmp_mask(mask, mask0, attr);
}

#if UNUSED
int lprint_key_mask_delim(char d) {
    return lprint_mask_delim(mask, d);
}
#endif

int lprint_key_mask() {
    return lprint_mask(mask);
}


/* The ignored key mask */
int init_ign_mask() {
    return init_mask(&ignmask);
}

void free_ign_mask() {
    free_mask(&ignmask);
}

#if UNUSED
void clear_ign_mask() {
    clear_mask(&ignmask);
}

int set_ign_bit(int bit, int val) {
    return set_bit(ignmask, bit, val);
}
#endif

int get_ign_bit(int bit) {
    return get_bit(ignmask, bit);
}

#if UNUSED
int cmp_ign_mask(unsigned char *mask0, unsigned int attr) {
    return cmp_mask(ignmask, mask0, attr);
}

int lprint_ign_mask_delim(char d) {
    return lprint_mask_delim(ignmask, d);
}

int lprint_ign_mask() {
    return lprint_mask(ignmask);
}
#endif

void copy_key_to_ign_mask() {
    memcpy(ignmask, mask, masksize);
}
