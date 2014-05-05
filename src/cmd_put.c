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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dplsh.h"

#include "libgen.h"

int cmd_put(int argc, char **argv);

struct usage_def put_usage[] =
  {
    {'a', USAGE_PARAM, "canned_acl", "default is private"},
    {'A', 0u, NULL, "list available canned acls"},
    {'m', USAGE_PARAM, "metadata", "comma or semicolon separated list of variables e.g. var1=val1[;|,]var2=val2;..."},
    {'q', USAGE_PARAM, "query_params", "comma or semicolon separated list of variables e.g. var1=val1[;|,]var2=val2;..."},
    {'P', 0u, NULL, "do a post"},
    {'O', 0u, NULL, "blob mode"},
    {USAGE_NO_OPT, USAGE_MANDAT, "local_file", "local file"},
    {USAGE_NO_OPT, 0u, "path", "remote file"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def put_cmd = {"put", "put file", put_usage, cmd_put};

int
cmd_put(int argc,
        char **argv)
{
  int ret;
  char opt;
  dpl_canned_acl_t canned_acl = DPL_CANNED_ACL_UNDEF;
  int Aflag = 0;
  int i;
  int fd = -1;
  dpl_dict_t *metadata = NULL;
  dpl_dict_t *query_params = NULL;
  char *local_file = NULL;
  char *remote_file = NULL;
  dpl_vfile_t *vfile = NULL;
  size_t block_size = 64*1024;
  ssize_t cc;
  struct stat st;
  char *buf = NULL;
  int retries = 0;
  int Pflag = 0;
  dpl_vfile_flag_t flags = 0u;
  int Oflag = 0;
  dpl_sysmd_t sysmd;
  char *data_addr = (char *) -1;
  off_t offset = 0;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, usage_getoptstr(put_usage))) != -1)
    switch (opt)
      {
      case 'm':
        metadata = dpl_parse_metadata(optarg);
        if (NULL == metadata)
          {
            fprintf(stderr, "error parsing metadata\n");
            return SHELL_CONT;
          }
        break ;
      case 'q':
        query_params = dpl_parse_metadata(optarg);
        if (NULL == metadata)
          {
            fprintf(stderr, "error parsing query_params\n");
            return SHELL_CONT;
          }
        break ;
      case 'a':
        canned_acl = dpl_canned_acl(optarg);
        if (-1 == canned_acl)
          {
            fprintf(stderr, "bad canned acl '%s'\n", optarg);
            return SHELL_CONT;
          }
        break ;
      case 'A':
        Aflag = 1;
        break ;
      case 'P':
        Pflag = 1;
        break ;
      case 'O':
        Oflag = 1;
        break ;
      case '?':
      default:
        usage_help(&put_cmd);
      return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  if (1 == Aflag)
    {
      for (i = 0;i < DPL_N_CANNED_ACL;i++)
        printf("%s\n", dpl_canned_acl_str(i));
      return SHELL_CONT;
    }

  if (2 == argc)
    {
      char *p, *p2;

      if (! strcmp("s3", (char *) dpl_get_backend_name(ctx)))
        {
          if (-1 == path_contains_valid_bucketname(ctx, argv[1]))
            {
              fprintf(stderr, "You need to set a bucket to use 'put', "
                      "or use 'la' to list the buckets\n");
              goto end;
            }
        }

      local_file = argv[0];
      remote_file = argv[1];

      p = index(remote_file, ':');
      if (NULL != p)
        {
          p++;
          if (!strcmp(p, ""))
            {
              p2 = rindex(local_file, '/');
              if (NULL != p2)
                {
                  p2++;
                  strcat(remote_file, p2);
                }
              else
                {
                  strcat(remote_file, local_file);
                }
            }
        }
    }
  else if (1 == argc)
    {
      local_file = argv[0];
      remote_file = rindex(local_file, '/');
      if (NULL != remote_file)
        remote_file++;
      else
        remote_file = local_file;
    }
  else
    {
      usage_help(&put_cmd);
      return SHELL_CONT;
    }

  /* ok, now if remote_file = <whatever>/. or . or .., or ...
   * just substitute with the correct destination path */
  char tmp_path[1024] = "";

  /* check if the destination is an existing dir; if so,
   * append the local file name to the remote directory one */
  memset(&sysmd, 0, sizeof sysmd);
  ret = dpl_getattr(ctx, remote_file, NULL, &sysmd);
  if (DPL_SUCCESS == ret)
    {
      if (sysmd.mask & DPL_SYSMD_MASK_FTYPE &&
          (DPL_FTYPE_DIR == sysmd.ftype ||
           DPL_FTYPE_SYMLINK == sysmd.ftype))
        {
          snprintf(tmp_path, sizeof tmp_path, "%s/%s",
                   remote_file, basename(local_file));
          remote_file = &tmp_path[0];
        }
    }

  memset(&sysmd, 0, sizeof (sysmd));

  if (DPL_CANNED_ACL_UNDEF != canned_acl)
    {
      sysmd.mask = DPL_SYSMD_MASK_CANNED_ACL;
      sysmd.canned_acl = canned_acl;
    }

  fd = open(local_file, O_RDONLY);
  if (-1 == fd)
    {
      perror("open");
      goto end;
    }

  ret = fstat(fd, &st);
  if (-1 == ret)
    {
      perror("fstat");
      return SHELL_CONT;
    }

  if (Oflag)
    {
      data_addr = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      if ((char *) -1 == data_addr)
	{
	  perror("mmap");
	  return SHELL_CONT;
	}
    }
  else
    {
      buf = malloc(block_size);
      if (NULL == buf)
	{
	  perror("malloc");
	  return SHELL_CONT;
	}
      
      flags = DPL_VFILE_FLAG_CREAT;

      ret = dpl_open(ctx,
		     remote_file,
		     flags,
		     NULL,
		     NULL,
		     metadata,
		     &sysmd,
		     query_params,
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
      return SHELL_CONT;
    }

  retries++;

  if (Oflag)
    {
      ret = dpl_fput(ctx,
		     remote_file,
		     NULL,
		     NULL,
		     NULL,
		     metadata,
		     &sysmd,
		     data_addr,
		     st.st_size);
      if (DPL_SUCCESS != ret)
	{
	  goto retry;
	}

    }
  else
    {
      while (1)
	{
	  cc = pread(fd, buf, block_size, offset);
	  if (-1 == cc)
	    {
	      perror("read");
	      goto end;
	    }
	  
	  if (0 == cc)
	    {
	      break ;
	    }
	  
	  ret = dpl_pwrite(vfile, buf, cc, offset);
	  if (DPL_SUCCESS != ret)
	    {
	      fprintf(stderr, "write failed\n");
	      goto retry;
	    }
	  
	  offset += cc;
	  
	  if (1 == hash)
	    {
	      fprintf(stderr, "#");
	      fflush(stderr);
	    }
	}
      
    }

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  if (-1 != fd)
    close(fd);

  if (Oflag)
    {
      if ((char *)-1 != data_addr)
	munmap(data_addr, st.st_size);
    }
  else
    {
      free(buf);

      if (vfile)
	dpl_close(vfile);
    }

  if (metadata)
    dpl_dict_free(metadata);

  if (query_params)
    dpl_dict_free(query_params);

  return SHELL_CONT;
}
