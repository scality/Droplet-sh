/*
 * Droplets, high performance cloud storage client library
 * Copyright (C) 2010 Scality http://github.com/scality/Droplets
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __DPL_SH_H__
#define __DPL_SH_H__ 1

#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <sys/wait.h>
#include <fnmatch.h>
#include <droplets.h>
#include "xfuncs.h"
#include "vars.h"
#include "shell.h"
#include "cmds.h"

extern int optind;

extern dpl_ctx_t *ctx;
extern int status;

/* PROTO filecompl.c */
/* ./filecompl.c */
#endif
