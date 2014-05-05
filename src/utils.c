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

#include "dplsh.h"

void
list_object_types()
{
  int i;

  for (i = 1;i <= DPL_FTYPE_SYMLINK;i++)
    printf("%s ", dpl_object_type_str(i));
  printf("\n");
}

/**
 * ask for confirmation
 *
 * @param str
 *
 * @return 0 if 'Yes', else 1
 */
int
ask_for_confirmation(char *str)
{
  char buf[256];
  ssize_t cc;
  char *p;

  fprintf(stderr, "%s (please type 'Yes' to confirm) ", str);
  fflush(stderr);
  cc = read(0, buf, sizeof(buf));
  if (-1 == cc)
    {
      perror("read");
      exit(1);
    }

  buf[sizeof (buf) - 1] = 0;

  if ((p = index(buf, '\n')))
    *p = 0;

  if (!strcmp(buf, "Yes"))
    return 0;

  return 1;
}

int
write_all(int fd,
          char *buf,
          int len)
{
  ssize_t cc;
  int remain;

  remain = len;
  while (1)
    {
    again:
      cc = write(fd, buf, remain);
      if (-1 == cc)
        {
          if (EINTR == errno)
            goto again;

          return -1;
        }

      remain -= cc;
      buf += cc;

      if (0 == remain)
        return 0;
    }
}

int
read_all(int fd,
         char *buf,
         int len)
{
  ssize_t cc;
  int remain;

  remain = len;
  while (1)
    {
    again:
      cc = read(fd, buf, remain);
      if (-1 == cc)
        {
          if (EINTR == errno)
            goto again;

          return -1;
        }

      if (0 == cc && 0 != len)
        {
          return -2;
        }

      remain -= cc;
      buf += cc;

      if (0 == remain)
        {
          return 0;
        }
    }
}

int
read_fd(int fd,
        char **data_bufp,
        u_int *data_lenp)
{
  char buf[8192];
  ssize_t cc;
  char *data_buf = NULL;
  u_int data_len = 0;

  while (1)
    {
      cc = read(fd, buf, sizeof (buf));
      if (-1 == cc)
        {
          return -1;
        }

      if (0 == cc)
        {
          break ;
        }

      if (NULL == data_buf)
        {
          data_buf = malloc(cc);
          if (NULL == data_buf)
            return -1;
        }
      else
        {
          char *tmp = realloc(data_buf, data_len + cc);
          if (NULL == tmp)
            {
              free(data_buf);
              return -1;
            }
          data_buf = tmp;
        }

      memcpy(data_buf + data_len, buf, cc);
      data_len += cc;
    }

  if (NULL != data_bufp)
    *data_bufp = data_buf;

  if (NULL != data_lenp)
    *data_lenp = data_len;

  return 0;
}

#if defined(__APPLE__) && defined(__MACH__) || defined(__ellcc__ )

size_t getline(char **lineptr, size_t *n, FILE *stream) {
    char *bufptr = NULL;
    char *p = bufptr;
    size_t size;
    int c;

    if (lineptr == NULL) {
        return -1;
    }
    if (stream == NULL) {
        return -1;
    }
    if (n == NULL) {
        return -1;
    }
    bufptr = *lineptr;
    size = *n;

    c = fgetc(stream);
    if (c == EOF) {
        return -1;
    }
    if (bufptr == NULL) {
        bufptr = malloc(128);
        if (bufptr == NULL) {
                return -1;
        }
        size = 128;
    }
    p = bufptr;
    while(c != EOF) {
        if ((p - bufptr) > (size - 1)) {
                size = size + 128;
                bufptr = realloc(bufptr, size);
                if (bufptr == NULL) {
                        return -1;
                }
        }
        *p++ = c;
        if (c == '\n') {
                break;
        }
        c = fgetc(stream);
    }

    *p++ = '\0';
    *lineptr = bufptr;
    *n = size;

    return p - bufptr - 1;
}

#endif

extern int optind;

int
linux_getopt(int argc, char * const argv[],
	      const char *optstring) {

  int rc = getopt(argc, argv, optstring);
  
#if defined(__APPLE__) && defined(__MACH__) || defined(__ellcc__ )
  // getopt OSX doesn't eat argv[0]
  optind++;
#endif
  return rc;
}

int
path_contains_valid_bucketname(dpl_ctx_t *ctx,
                               const char * const path)
{
  int ret;

  /* we can do 'cmd bucket:/path' or 'cmd bucket:', even if no bucket is set */
  if (path)
    {
      char *p = strchr(path, ':');
      dpl_vec_t *buckets = NULL;
      int i;
      int found = 0;

      if (! p)
        goto no_bucket;

      *p = 0;

      ret = dpl_list_all_my_buckets(ctx, &buckets);
      if (DPL_SUCCESS != ret)
        goto no_bucket;

      for (i = 0; i < buckets->n_items && 0 == found; i++)
        {
          dpl_value_t *item = dpl_vec_get(buckets, i);
          if (item)
            {
              if (! strcmp(path, (char *) item->string))
                found = 1;
            }
        }

      *p = ':';

      if (0 == found)
        goto no_bucket;
    }
  else
    {
    no_bucket:
      if (! ctx->cur_bucket || ! strcmp(ctx->cur_bucket, ""))
        {
          ret = -1;
          goto end;
        }
    }

  ret = 0;
 end:
  return ret;
}
