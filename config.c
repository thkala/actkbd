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


#ifndef CONFIG
#define CONFIG "/etc/actkbd.conf"
#endif


static int strtolower(char *str) {
    int i, l;

    if (str == NULL)
	return INTERR;

    l = strlen(str);
    for (i = 0; i < l; ++i)
	str[i] = tolower(str[i]);

    return OK;
}


static int proc_config(int lineno, char *line, key_cmd **cmd) {
    int i, l, f = 1, eventno = INVALID, ret = CONFERR;
    char *dup = NULL, *event = NULL, *attrs = NULL, *command = NULL, *err = NULL;
    unsigned char *keys;
    unsigned int attr_bits = 0;
    attr_t *attrlst = NULL, *attr = NULL, *attr_last = NULL;

    l = strlen(line);
    for (i = 0; i <= l; ++i) {
    	if (((line[i] == '#') || (line[i] == '\n') || (line[i] == '\0')) && (f < 4)) {
	    err = "missing fields";
	    goto ERROR;
	}
	if ((line[i] == ':') && (f < 4)) {
	    switch (f) {
		case 1:
		    event = line + i + 1;
		    break;
		case 2:
		    attrs = line + i + 1;
		    break;
		case 3:
		    command = line + i + 1;
		    break;
	    }
	    ++f;
	}
    }

    /* Keep a copy of the line */
    dup = strdup(line);
    if (dup == NULL) {
	lprintf("Error: memory allocation failed\n");
	ret = MEMERR;
	goto ERROR;
    }

    /* Set the field boundaries */
    line[event - line - 1] = '\0';
    line[attrs - line - 1] = '\0';
    line[command - line - 1] = '\0';
    if (line[l - 1] == '\n')
	line[l - 1] = '\0';

    /* Set the event type */
    strtolower(event);
    if (strcmp(event, "") == 0)
	eventno = KEY;
    else if (strcmp(event, "key") == 0)
	eventno = KEY;
    else if (strcmp(event, "rep") == 0)
	eventno = REP;
    else if (strcmp(event, "rel") == 0)
	eventno = REL;

    if (eventno == INVALID) {
	err = "invalid event type";
	goto ERROR;
    }

    /* The keys are always at the beginning of the line */
    if (strmask(&keys, line) != OK) {
	err = "invalid key field";
	goto ERROR;
    }

    /* Set the module list */
    strtolower(attrs);
    while ((event = strsep(&attrs, ", \t")) != NULL) {
	int type = -1;
	void *opt = NULL;
	char *num = NULL;

	if (strlen(event) == 0)
	    continue;

	if (strcmp(event, "noexec") == 0) {
	    attr_bits |= BIT_ATTR_NOEXEC;
	} else if (strcmp(event, "grabbed") == 0) {
	    attr_bits |= BIT_ATTR_GRABBED;
	} else if (strcmp(event, "ungrabbed") == 0) {
	    attr_bits |= BIT_ATTR_UNGRABBED;
	} else if (strcmp(event, "any") == 0) {
	    attr_bits |= BIT_ATTR_ANY;
	} else if (strcmp(event, "all") == 0) {
	    attr_bits |= BIT_ATTR_ALL;
	} else if (strcmp(event, "exec") == 0) {
	    type = ATTR_EXEC;
	} else if (strcmp(event, "grab") == 0) {
	    type = ATTR_GRAB;
	} else if (strcmp(event, "ungrab") == 0) {
	    type = ATTR_UNGRAB;
	} else if (strcmp(event, "ignrel") == 0) {
	    type = ATTR_IGNREL;
	} else if (strcmp(event, "rcvrel") == 0) {
	    type = ATTR_RCVREL;
	} else if (strcmp(event, "allrel") == 0) {
	    type = ATTR_ALLREL;
	} else if (strncmp(event, "key(", 4) == 0) {
	    type = ATTR_KEY;
	    event += 4;
	    num = strsep(&event, "()");
	} else if (strncmp(event, "rel(", 4) == 0) {
	    type = ATTR_REL;
	    event += 4;
	    num = strsep(&event, "()");
	} else if (strncmp(event, "rep(", 4) == 0) {
	    type = ATTR_REP;
	    event += 4;
	    num = strsep(&event, "()");
	} else if (strncmp(event, "ledon(", 6) == 0) {
	    type = ATTR_LEDON;
	    event += 6;
	    num = strsep(&event, "()");
	} else if (strncmp(event, "ledoff(", 7) == 0) {
	    type = ATTR_LEDOFF;
	    event += 7;
	    num = strsep(&event, "()");
	} else {
	    lprintf("Warning: unknown attribute %s\n", event);
	}

	if (num != NULL) {
	    errno = 0;
	    if (strlen(num) > 0) {
		opt = (void *)((int)strtol(num, (char **)NULL, 10));
	    } else {
		opt = (void *)((int)(-1));
	    }

	    if (((int)opt < 0) &&
		    ((type == ATTR_LEDON) || (type == ATTR_LEDOFF)))
		errno = EINVAL;

	    if (errno != 0) {
		err = "invalid attribute argument";
		goto ERROR;
	    }
	}

	if (type != -1) {
	    attr = (attr_t *)(malloc(sizeof(attr_t)));
	    if (attr == NULL) {
		lprintf("Error: memory allocation failed\n");
		ret = MEMERR;
		goto ERROR;
	    }

	    attr->type = type;
	    attr->opt = opt;
	    attr->next = NULL;

	    if (attrlst == NULL) {
		attrlst = attr;
		attr_last = attr;
	    } else {
		attr_last->next = attr;
		attr_last = attr;
	    }
	}
    }

    *cmd = (key_cmd *)(malloc(sizeof(key_cmd)));
    if (*cmd == NULL) {
	lprintf("Error: memory allocation failed\n");
	ret = MEMERR;
	goto ERROR;
    } else {
	(*cmd)->keys = keys;
	(*cmd)->type = eventno;
	(*cmd)->command = strdup(command);
	(*cmd)->attr_bits = attr_bits;
	(*cmd)->attrs = attrlst;
    }

    /* Destroy the line copy */
    free(dup);

    return OK;

    /* Error handler */
ERROR:
    if ((verbose > 1) && (err != NULL))
	lprintf("Warning: %s in configuration line %i\n", err, lineno);
    if (verbose > 0)
	lprintf("Warning: discarding configuration line %i:\n\t%s%c", lineno,
		(dup != NULL)?dup:line,
		(((dup != NULL)?dup:line)[l - 1] == '\n')?'\0':'\n');

    *cmd = NULL;

    /* Free the attribute list */
    while (attrlst != NULL) {
	attr = attrlst;
	attrlst = attrlst->next;
	free(attr);
    }

    if (dup != NULL)
	free(dup);

    return ret;
}


typedef struct _confentry {
    key_cmd *cmd;
    struct _confentry *next;
} confentry;


static confentry *list = NULL;


static void print_attrs(key_cmd *cmd) {
    attr_t *attr;
    char *sep = "";

    if (cmd == NULL)
	return;

    if ((cmd->attr_bits & BIT_ATTR_NOEXEC) > 0) {
	lprintf("%snoexec", sep);
	sep = ",";
    }
    if ((cmd->attr_bits & BIT_ATTR_GRABBED) > 0) {
	lprintf("%sgrabbed", sep);
	sep = ",";
    }
    if ((cmd->attr_bits & BIT_ATTR_UNGRABBED) > 0) {
	lprintf("%sungrabbed", sep);
	sep = ",";
    }
    if ((cmd->attr_bits & BIT_ATTR_ANY) > 0) {
	lprintf("%sany", sep);
	sep = ",";
    }
    if ((cmd->attr_bits & BIT_ATTR_ALL) > 0) {
	lprintf("%sall", sep);
	sep = ",";
    }

    attr = cmd->attrs;
    while (attr != NULL) {
	char *str = "";
	char opt[32] = { '\0' };
	switch (attr->type) {
	    case ATTR_EXEC:
		str = "exec";
		break;
	    case ATTR_GRAB:
		str = "grab";
		break;
	    case ATTR_UNGRAB:
		str = "ungrab";
		break;
	    case ATTR_IGNREL:
		str = "ignrel";
		break;
	    case ATTR_RCVREL:
		str = "rcvrel";
		break;
	    case ATTR_ALLREL:
		str = "allrel";
		break;
	    case ATTR_KEY:
		snprintf(opt, 32, "key(%i)", (int)(attr->opt));
		break;
	    case ATTR_REL:
		snprintf(opt, 32, "rel(%i)", (int)(attr->opt));
		break;
	    case ATTR_REP:
		snprintf(opt, 32, "rep(%i)", (int)(attr->opt));
		break;
	    case ATTR_LEDON:
		snprintf(opt, 32, "ledon(%i)", (int)(attr->opt));
		break;
	    case ATTR_LEDOFF:
		snprintf(opt, 32, "ledoff(%i)", (int)(attr->opt));
		break;
	    default:
		str = "unknown";
		break;
	}
	lprintf("%s%s%s", sep, str, opt);
	attr = attr->next;
	sep = ",";
    }
}


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
	lprintf("Warning: could not open the configuration file %s: %s\n", config, strerror(errno));
	return OK;
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
		lprintf(" -:- %s -:- ", (cmd->type == KEY)?"key":((cmd->type == REP)?"rep":"rel"));
		print_attrs(cmd);
		lprintf(" -:- %s\n", cmd->command);
	    }
	}
	free(line);
	++lineno;
    }

    fclose(fp);

    return OK;
}


int close_config() {
    confentry *node = list;
    void *tmp;
    attr_t *attr;

    while (node != NULL) {
	free_mask(&(node->cmd->keys));
	free(node->cmd->command);

	/* Free the attribute list */
	attr = node->cmd->attrs;
	while (attr != NULL) {
	    tmp = attr;
	    attr = attr->next;
	    free(tmp);
	}

	free(node->cmd);
	tmp = node->next;
	free(node);
	node = tmp;
    }
    list = NULL;

    return OK;
}


int match_key(int type, key_cmd **command) {
    confentry *node = list;

    *command = NULL;

    while (node != NULL) {
	if ((node->cmd->type != type) ||
		(((node->cmd->attr_bits & BIT_ATTR_GRABBED) > 0) && (!grabbed)) ||
		(((node->cmd->attr_bits & BIT_ATTR_UNGRABBED) > 0) && (grabbed))) {
	    node = node->next;
	    continue;
	}
	if (cmp_key_mask(node->cmd->keys,
		(node->cmd->attr_bits & BIT_ATTR_ANY) > 0,
		(node->cmd->attr_bits & BIT_ATTR_ALL) > 0)) {
	    *command = node->cmd;
	    return OK;
	}
	node = node->next;
    }

    return NOMATCH;
}
