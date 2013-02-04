/*
 * Droplet, high performance cloud storage client library
 * Copyright (C) 2010 Scality http://github.com/scality/Droplet
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

#ifndef __DPLSH_H__
#define __DPLSH_H__ 1

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
#include <fcntl.h>
#if defined (HAVE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#else
#include <editline/readline.h>
#endif /* HAVE_READLINE */
#include <droplet.h>
#include <droplet/vfs.h>
#include "xfuncs.h"
#include "usage.h"
#include "vars.h"
#include "shell.h"
#include "utils.h"

#define DPLSH_VERSION_MAJOR 1
#define DPLSH_VERSION_MINOR 0

#define DPLSHRC ".dplshrc"

struct ls_data
{
  dpl_ctx_t *ctx;
  int lflag; //print long
  int Rflag; //recursive (vdir)
  int Xflag; //list all (raw)
  int pflag; //do not print
  int dflag; //getattr on directory?
  size_t total_size;
};

#include "cmds.h"

extern int optind;

extern dpl_ctx_t *ctx;
extern int status;
extern u_int block_size;
extern int hash;

/* PROTO dplsh.c filecompl.c */
/* ./dplsh.c */
int do_quit(void);
char *var_set_status(char *value);
char *var_set_trace_level(char *value);
char *var_set_block_size(char *value);
char *var_set_hash(char *value);
char *var_set_delim(char *value);
int main(int argc, char **argv);
/* ./filecompl.c */
void *do_opendir(char *path);
dpl_dirent_t *do_readdir(void *dir_hdl);
void do_closedir(void *dir_hdl);
char *file_completion(const char *text, int state);
#endif
