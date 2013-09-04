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
#include <unistd.h>
#include "dplsh.h"

#include "libgen.h"

int cmd_put(int argc, char **argv);

struct usage_def put_usage[] =
  {
    {'k', 0u, NULL, "encrypt file"},
    {'M', 0u, NULL, "compute MD5"},
    {'a', USAGE_PARAM, "canned_acl", "default is private"},
    {'A', 0u, NULL, "list available canned acls"},
    {'m', USAGE_PARAM, "metadata", "comma or semicolon separated list of variables e.g. var1=val1[;|,]var2=val2;..."},
    {'q', USAGE_PARAM, "query_params", "comma or semicolon separated list of variables e.g. var1=val1[;|,]var2=val2;..."},
    {'P', 0u, NULL, "do a post"},
    {'O', 0u, NULL, "buffered"},
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
  int kflag = 0;
  char *buf = NULL;
  int retries = 0;
  int Pflag = 0;
  dpl_vfile_flag_t flags = 0u;
  int Oflag = 1;
  int Mflag = 0;
  dpl_sysmd_t sysmd;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, usage_getoptstr(put_usage))) != -1)
    switch (opt)
      {
      case 'k':
        kflag = 1;
        break ;
      case 'M':
        Mflag = 1;
        break ;
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
        Oflag = 0;
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
    block_size = st.st_size;

  buf = malloc(block_size);
  if (NULL == buf)
    {
      perror("malloc");
      return SHELL_CONT;
    }

  flags = DPL_VFILE_FLAG_CREAT;
  if (kflag)
    flags |= DPL_VFILE_FLAG_ENCRYPT;
  else if (Mflag)
    flags |= DPL_VFILE_FLAG_MD5;
  if (Pflag)
    flags |= DPL_VFILE_FLAG_POST;
  if (Oflag)
    flags |= DPL_VFILE_FLAG_BLOB;

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

  ret = dpl_openwrite(ctx,
                      remote_file,
                      DPL_FTYPE_REG,
                      flags,
                      NULL,
                      metadata,
                      &sysmd,
                      st.st_size,
                      query_params,
                      &vfile);
  if (DPL_SUCCESS != ret)
    {
      goto retry;
    }

  long long offset = 0;
  while (1)
    {
      cc = read(fd, buf, block_size);
      if (-1 == cc)
        {
          perror("read");
          goto end;
        }

      if (0 == cc)
        {
          break ;
        }

      ret = dpl_pwrite(vfile, buf, cc, offset, NULL, NULL);
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

  ret = dpl_close(vfile);
  if (DPL_SUCCESS != ret)
    {
      //fprintf(stderr, "close failed %s (%d)\n", dpl_status_str(ret), ret);
      goto retry;
    }

  vfile = NULL;

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  if (-1 != fd)
    close(fd);

  free(buf);

  if (metadata)
    dpl_dict_free(metadata);

  if (query_params)
    dpl_dict_free(query_params);

  if (vfile)
    dpl_close(vfile);

  return SHELL_CONT;
}
