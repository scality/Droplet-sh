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

int cmd_get(int argc, char **argv);

struct usage_def get_usage[] =
  {
    {'k', 0u, NULL, "encrypt file"},
    {'s', USAGE_PARAM, "start", "range start offset"},
    {'e', USAGE_PARAM, "end", "range end offset"},
    {'O', 0u, NULL, "buffered"},
    {USAGE_NO_OPT, USAGE_MANDAT, "path", "remote file"},
    {USAGE_NO_OPT, 0u, "local_file or - or |cmd", "local file"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def get_cmd = {"get", "get file", get_usage, cmd_get};

struct get_data
{
  int fd;
  FILE *pipe;
};

static dpl_status_t
cb_get_buffered(void *cb_arg,
                char *buf,
                u_int len)
{
  struct get_data *get_data = (struct get_data *) cb_arg;
  int ret;

  if (NULL != get_data->pipe)
    {
      size_t s;

      s = fwrite(buf, 1, len, get_data->pipe);
      if (s != len)
        {
          perror("fwrite");
          return DPL_FAILURE;
        }
    }
  else
    {
      if (1 == hash)
        {
          fprintf(stderr, "#");
          fflush(stderr);
        }

      ret = write_all(get_data->fd, buf, len);
      if (0 != ret)
        {
          ret = DPL_FAILURE;
          goto end;
        }
    }

  ret = DPL_SUCCESS;

 end:

  return ret;
}

int
cmd_get(int argc,
        char **argv)
{
  int ret;
  char opt;
  char *path = NULL;
  struct get_data get_data;
  int do_stdout = 0;
  int kflag = 0;
  char *local_file = NULL;
  dpl_range_t range;
  int start_inited = 0;
  int end_inited = 0;
  int retries = 0;
  int Oflag = 1;
  dpl_vfile_flag_t flags;

  memset(&get_data, 0, sizeof (get_data));
  get_data.fd = -1;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  range.start = range.end = DPL_UNDEF;
  while ((opt = linux_getopt(argc, argv, usage_getoptstr(get_usage))) != -1)
    switch (opt)
      {
      case 's':
        range.start = strtol(optarg, NULL, 0);
        start_inited = 1;
        break ;
      case 'e':
        range.end = strtol(optarg, NULL, 0);
        end_inited = 1;
        break ;
      case 'k':
        kflag = 1;
        break ;
      case 'O':
        Oflag = 0;
        break ;
      case '?':
      default:
        usage_help(&get_cmd);
        return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  if (start_inited != end_inited)
    {
      fprintf(stderr, "please provide -s and -e\n");
      return SHELL_CONT;
    }

  if (2 == argc)
    {
      path = argv[0];
      local_file = argv[1];
    }
  else if (1 == argc)
    {
      path = argv[0];
      local_file = rindex(path, '/');
      if (NULL != local_file)
        local_file++;
      else
        local_file = path;
    }
  else
    {
      usage_help(&get_cmd);
      return SHELL_CONT;
    }

  if (!strcmp(local_file, "-"))
    {
      get_data.fd = 1;
      do_stdout = 1;
    }
  else if ('|' == local_file[0])
    {
      get_data.fd = 1;
      do_stdout = 1;
      get_data.pipe = popen(local_file + 1, "w");
      if (NULL == get_data.pipe)
        {
          fprintf(stderr, "pipe failed\n");
          goto end;
        }
    }
  else
    {
      ret = access(local_file, F_OK);
      if (0 == ret)
        {
          if (1 == ask_for_confirmation("file already exists, overwrite?"))
            return SHELL_CONT;
        }

      get_data.fd = open(local_file, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      if (-1 == get_data.fd)
        {
          perror("open");
          goto end;
        }
    }

  flags = 0u;
  if (kflag)
    flags |= DPL_VFILE_FLAG_ENCRYPT;
  if (Oflag)
    flags |= DPL_VFILE_FLAG_ONESHOT;
  if (start_inited || end_inited)
    flags |= DPL_VFILE_FLAG_RANGE;

 retry:

  if (0 != retries)
    {
      if (1 == hash)
        {
          fprintf(stderr, "R");
          fflush(stderr);
        }
    }

  if (retries >= 3)
    {
      fprintf(stderr, "too many retries: %s (%d)\n", dpl_status_str(ret), ret);
      goto end;
    }

  retries++;

  ret = dpl_openread(ctx, path, flags, NULL, &range, cb_get_buffered, &get_data, NULL, NULL);
  if (DPL_SUCCESS != ret)
    {
      if (DPL_ENOENT == ret)
        {
          fprintf(stderr, "no such object\n");
          goto end;
        }
      goto retry;
    }

  var_set("status", "0", VAR_CMD_SET, NULL);
      
 end:

  if (0 == do_stdout)
    {
      if (-1 != get_data.fd)
        close(get_data.fd);
    }

  if (NULL != get_data.pipe)
    pclose(get_data.pipe);

  return SHELL_CONT;
}
