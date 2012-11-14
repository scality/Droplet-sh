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

#ifndef __UTILS_H__
#define __UTILS_H__ 1

/* PROTO utils.c */
/* ./utils.c */
void list_object_types();
int ask_for_confirmation(char *str);
int write_all(int fd, char *buf, int len);
int read_all(int fd, char *buf, int len);
int read_fd(int fd, char **data_bufp, u_int *data_lenp);

#if defined(__APPLE__) && defined(__MACH__) || defined(__ellcc__ )
size_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

int linux_getopt(int argc, char * const argv[],
		 const char *optstring);

#endif
