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
    {'f', 0u, NULL, "ignore local file"},
    {'s', USAGE_PARAM, "start", "range start offset"},
    {'e', USAGE_PARAM, "end", "range end offset"},
    {'O', 0u, NULL, "blob mode"},
    {USAGE_NO_OPT, USAGE_MANDAT, "path", "remote file"},
    {USAGE_NO_OPT, 0u, "local_file or - or |cmd", "local file"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def get_cmd = {"get", "get file", get_usage, cmd_get};

int
cmd_get(int argc,
        char **argv)
{
  int ret;
  char opt;
  char *path = NULL;
  int do_stdout = 0;
  char *local_file = NULL;
  dpl_range_t range;
  int start_inited = 0;
  int end_inited = 0;
  int retries = 0;
  int Oflag = 0;
  dpl_vfile_t *vfile = NULL;
  dpl_vfile_flag_t flags;
  int ignore_local_file = 0;
  int fd = -1;
  FILE *pipe = NULL;
  char *buf = NULL;
  int len;
  off_t offset = 0;
	  
  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  range.start = range.end = DPL_UNDEF;
  while ((opt = linux_getopt(argc, argv, usage_getoptstr(get_usage))) != -1)
    switch (opt)
      {
      case 'f':
        ignore_local_file = 1;
        break;
      case 's':
        range.start = strtol(optarg, NULL, 0);
        start_inited = 1;
        break ;
      case 'e':
        range.end = strtol(optarg, NULL, 0);
        end_inited = 1;
        break ;
      case 'O':
        Oflag = 1;
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

  if (! strcmp("s3", (char *) dpl_get_backend_name(ctx)))
    {
      if (-1 == path_contains_valid_bucketname(ctx, path))
        {
          fprintf(stderr, "You need to set a bucket to use 'get' "
                  "(use 'la' to list the buckets)\n");
          goto end;
        }
    }

  if (!strcmp(local_file, "-"))
    {
      fd = 1;
      do_stdout = 1;
    }
  else if ('|' == local_file[0])
    {
      fd = 1;
      do_stdout = 1;
      pipe = popen(local_file + 1, "w");
      if (NULL == pipe)
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
          if (0 == ignore_local_file)
            {
              if (1 == ask_for_confirmation("file already exists, overwrite?"))
                return SHELL_CONT;
            }
        }

      fd = open(local_file, O_WRONLY|O_CREAT|O_TRUNC, 0600);
      if (-1 == fd)
        {
          perror("open");
          goto end;
        }
    }

  if (Oflag)
    {
    }
  else
    {
      flags = 0u;

      ret = dpl_open(ctx,
		     path,
		     flags,
		     NULL,
		     NULL,
		     NULL,
		     NULL,
		     NULL,
		     &vfile);
      if (DPL_SUCCESS != ret)
	{
	  goto retry;
	}

      offset = 0;
    }

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

  if (Oflag)
    {
      ret = dpl_fget(ctx,
		     path,
		     NULL,
		     NULL, 
		     (start_inited || end_inited) ? &range : NULL,
		     &buf,
		     &len,
		     NULL,
		     NULL);
      if (DPL_SUCCESS != ret)
	{
	  if (DPL_ERANGEUNAVAIL == ret)
	    {
	      /* ran off the end */
	      goto end;
	    }
	  if (DPL_ENOENT == ret)
	    {
	      fprintf(stderr, "no such object\n");
	      goto end;
	    }
	  goto retry;
	}

      if (NULL != pipe)
	{
	  size_t s;

	  s = fwrite(buf, 1, len, pipe);
	  if (s != len)
	    {
	      perror("fwrite");
	      return SHELL_CONT;
	    }
	}
      else
	{
	  if (1 == hash)
	    {
	      fprintf(stderr, "#");
	      fflush(stderr);
	    }

	  ret = write_all(fd, buf, len);
	  if (0 != ret)
	    {
	      return SHELL_CONT;
	    }
	}
    }
  else
    {
      while (1)
	{
	  int cc;

	  ret = dpl_pread(vfile, block_size, offset, &buf, &len);
	  if (DPL_SUCCESS != ret)
	    {
	      if (DPL_ENOENT == ret || DPL_ERANGEUNAVAIL == ret)
		break ;

	      fprintf(stderr, "pread failed %s (%d)\n", dpl_status_str(ret), ret);
	      goto retry;
	    }

	  if (NULL != pipe)
	    {
	      size_t s;
	      
	      s = fwrite(buf, 1, len, pipe);
	      if (s != len)
		{
		  perror("fwrite");
		  return SHELL_CONT;
		}
	    }
	  else
	    {
	      if (1 == hash)
		{
		  fprintf(stderr, "#");
		  fflush(stderr);
		}
	      
	      ret = write_all(fd, buf, len);
	      if (0 != ret)
		{
		  return SHELL_CONT;
		}
	    }
  
	  free(buf);
	  buf = NULL;
	  offset += len;
	  
	  if (1 == hash)
	    {
	      fprintf(stderr, "#");
	      fflush(stderr);
	    }
	}
     
    }


  var_set("status", "0", VAR_CMD_SET, NULL);
      
 end:

  if (0 == do_stdout)
    {
      if (-1 != fd)
        close(fd);
    }

  if (NULL != pipe)
    pclose(pipe);

  if (Oflag)
    {
    }
  else
    {
      ret = dpl_close(vfile);
      if (DPL_SUCCESS != ret)
	{
	  //fprintf(stderr, "close failed %s (%d)\n", dpl_status_str(ret), ret);
	  goto retry;
	}
    }

  free(buf);

  return SHELL_CONT;
}
