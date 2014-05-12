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


#define CONFIG "/etc/actkbd.conf"


static int strtolower(char *str) {
    int i, l;

    if (str == NULL)
	return INTERR;

    l = strlen(str);
    for (i = 0; i < l; ++i)
	str[i] = tolower(str[i]);

    return OK;
}


static int setmask(unsigned char **mask, char *keys) {
    int l, i, k;

    /* Default mask pointer */
    *mask = NULL;

    l = strlen(keys);

    /* Clean up the input */
    for (i = 0; i < l; ++i) {
	if ((keys[i] == '+') || (keys[i] == '-'))
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


static int proc_config(int lineno, char *line, key_cmd **cmd) {
    int i, l, f = 1, eventno = INVALID, ret = OK;
    char *dup = NULL, *event = NULL, *module = NULL, *command = NULL;
    unsigned char *keys;

    /* Keep a copy of the line */
    dup = strdup(line);

    l = strlen(line);
    for (i = 0; i <= l; ++i) {
    	if (((line[i] == '#') || (line[i] == '\n') || (line[i] == '\0')) && (f < 4)) {
	    if (verbose > 1)
		lprintf("Warning: missing fields in configuration line %i\n", lineno);
	    ret = CONFERR;
	    break;
	}
	if ((line[i] == ':') && (f < 4)) {
	    switch (f) {
		case 1:
		    event = line + i + 1;
		    break;
		case 2:
		    module = line + i + 1;
		    break;
		case 3:
		    command = line + i + 1;
		    break;
	    }
	    ++f;
	}
    }

    if (ret == OK) {
	line[event - line - 1] = '\0';
	line[module - line - 1] = '\0';
	line[command - line - 1] = '\0';

	if (line[l - 1] == '\n')
	    line[l - 1] = '\0';

	strtolower(event);
	strtolower(module);

	if (strcmp(event, "") == 0)
	    eventno = KEY;
	if (strcmp(event, "key") == 0)
	    eventno = KEY;
	if (strcmp(event, "rep") == 0)
	    eventno = REP;
	if (strcmp(event, "rel") == 0)
	    eventno = REL;

	if ((verbose > 1) && (eventno == INVALID))
	    lprintf("Warning: invalid event type in configuration line %i\n", lineno);

	if (eventno != INVALID) {
	    /* The keys are always at the beginning of the line */
	    if (setmask(&keys, line) != OK) {
		if (verbose > 1)
		    lprintf("Warning: invalid key field in configuration line %i\n", lineno);
		ret = CONFERR;
	    }
	} else {
	    ret = CONFERR;
	}
    }

    if (ret != OK) {
	if (verbose > 0)
	    lprintf("Warning: discarding configuration line %i:\n\t%s%c", lineno, dup, (dup[l - 1] == '\n')?'\0':'\n');
	*cmd = NULL;
    } else {
	*cmd = (key_cmd *)(malloc(sizeof(key_cmd)));
	if (*cmd == NULL) {
	    lprintf("Error: memory allocation failed\n");
	    ret = MEMERR;
	} else {
	    (*cmd)->keys = keys;
	    (*cmd)->type = eventno;
	    (*cmd)->module = strdup(module);
	    (*cmd)->command = strdup(command);
	}
    }

    /* Destroy the line copy */
    free(dup);

    return ret;
}


typedef struct _confentry {
    key_cmd *cmd;
    struct _confentry *next;
} confentry;


static confentry *list = NULL;


int open_config() {
    FILE *fp = NULL;
    char *line;
    key_cmd *cmd;
    confentry *lastnode = NULL, *newnode = NULL;
    int lineno = 1, n = 0, ret = 0;

    /* Allow the configuration file to be overridden */
    if (!config)
	config = CONFIG;

    fp = fopen(config, "r");
    if (fp == NULL) {
	lprintf("Error: could not open the configuration file %s: %s\n", config, strerror(errno));
	return CONFERR;
    }
    if (verbose > 1)
	lprintf("Using configuration file %s\n", config);

    while (!feof(fp) && (ret >=0)) {
	line = NULL;
	ret = getline(&line, &n, fp);
	if ((ret > 0) && (proc_config(lineno, line, &cmd) == OK)) {
	    newnode = (confentry *)(malloc(sizeof(confentry)));
	    if (newnode == NULL) {
		lprintf("Error: memory allocation failed\n");
		close_config();
		free(line);
		return MEMERR;
	    }

	    newnode->cmd = cmd;
	    newnode->next = NULL;

	    if (list == NULL) {
		list = newnode;
	    } else {
		lastnode->next = newnode;
	    }
	    lastnode = newnode;

	    if (verbose > 1) {
		lprintf("Config: ");
		lprint_mask(cmd->keys);
		lprintf(" -:- %s -:- %s -:- %s\n", (cmd->type == KEY)?"key":((cmd->type == REP)?"rep":"rel"), cmd->module, cmd->command);
	    }
	}
	free(line);
	++lineno;
    }

    return OK;
}


int close_config() {
    confentry *node = list, *tmp;
    while (node != NULL) {
	clear_mask(&(node->cmd->keys));
	free(node->cmd->module);
	free(node->cmd->command);
	free(node->cmd);
	tmp = node->next;
	free(node);
	node = tmp;
    }
    list = NULL;
    return OK;
}


int get_command(unsigned char *mask, int type, char **command) {
    confentry *node = list;

    *command = NULL;

    while (node != NULL) {
	if ((node->cmd->type == type) && (cmp_mask(mask, node->cmd->keys) == 0)) {
	    *command = node->cmd->command;
	    return OK;
	}
	node = node->next;
    }

    return NOMATCH;
}
