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

int cmd_rm(int argc, char **argv);

struct usage_def rm_usage[] =
  {
    {'r', 0u, NULL, "recurse"},
    {'f', 0u, NULL, "force"},
    {'v', 0u, NULL, "verbose"},
    {USAGE_NO_OPT, USAGE_MANDAT, "path", "remote file"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def rm_cmd = {"rm", "remove file", rm_usage, cmd_rm};

dpl_status_t
recurse(dpl_ctx_t *ctx,
        char *dir,
        int verbose,
        int level)
{
  void *dir_hdl;
  dpl_dirent_t dirent;
  int ret;

  ret = dpl_chdir(ctx, dir);
  if (DPL_SUCCESS != ret)
    return ret;

  ret = dpl_opendir(ctx, ".", &dir_hdl);
  if (DPL_SUCCESS != ret)
    return ret;

  while (!dpl_eof(dir_hdl))
    {
      ret = dpl_readdir(dir_hdl, &dirent);
      if (DPL_SUCCESS != ret)
        return ret;

      if (strcmp(dirent.name, "."))
        {
          int i;

          if (DPL_FTYPE_DIR == dirent.type)
            {
              ret = recurse(ctx, dirent.name, verbose, level + 1);
              if (DPL_SUCCESS != ret)
                return ret;
            }

          if (verbose)
            {
              for (i = 0;i < level;i++)
                printf(" ");
              printf("%s\n", dirent.name);
            }

          if (DPL_FTYPE_DIR == dirent.type)
            ret = dpl_rmdir(ctx, dirent.name);
          else
            ret = dpl_unlink(ctx, dirent.name);

          if (DPL_SUCCESS != ret)
            {
              fprintf(stderr, "status: %s (%d)\n", dpl_status_str(ret), ret);
              return ret;
            }
        }
    }

  dpl_closedir(dir_hdl);

  ret = dpl_chdir(ctx, "..");
  if (DPL_SUCCESS != ret)
    return ret;

  return DPL_SUCCESS;
}

int
cmd_rm(int argc,
       char **argv)
{
  int ret;
  char opt;
  char *path = NULL;
  int rflag = 0;
  __attribute__((unused)) int fflag = 0;
  int verbose = 0;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, usage_getoptstr(rm_usage))) != -1)
    switch (opt)
      {
      case 'r':
        rflag = 1;
        break ;
      case 'f':
        fflag = 1;
        break ;
      case 'v':
        verbose = 1;
        break ;
      case '?':
      default:
        usage_help(&rm_cmd);
        return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  if (1 != argc)
    {
      usage_help(&rm_cmd);
      return SHELL_CONT;
    }

  path = argv[0];

  if (rflag)
    {
      ret = recurse(ctx, path, verbose, 0);
      if (DPL_SUCCESS != ret &&
          (0 == fflag || (DPL_ENOENT != ret && 1 == fflag)))
        {
          fprintf(stderr, "error recursing: %s (%d)\n", dpl_status_str(ret), ret);
          ret = 1;
          goto end;
        }

      ret = dpl_rmdir(ctx, path);
      if (DPL_SUCCESS != ret &&
          (0 == fflag || (DPL_ENOENT != ret && 1 == fflag)))
        {
          fprintf(stderr, "status: %s (%d)\n", dpl_status_str(ret), ret);
          goto end;
        }
    }
  else
    {
      ret = dpl_unlink(ctx, path);
      if (DPL_SUCCESS != ret &&
          (0 == fflag || (DPL_ENOENT != ret && 1 == fflag)))
        {
          fprintf(stderr, "status: %s (%d)\n", dpl_status_str(ret), ret);
          goto end;
        }
    }

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  return SHELL_CONT;
}
