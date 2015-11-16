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

int cmd_getattr(int argc, char **argv);

struct usage_def getattr_usage[] =
  {
    {'r', 0u, NULL, "raw getattr"},
    {'s', 0u, NULL, "get sysmd"},
    {USAGE_NO_OPT, USAGE_MANDAT, "path", "remote object"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def getattr_cmd = {"getattr", "dump attributes of object", getattr_usage, cmd_getattr};

int
cmd_getattr(int argc,
         char **argv)
{
  int ret;
  char opt;
  char *path = NULL;
  dpl_dict_t *metadata = NULL;
  dpl_sysmd_t sysmd;
  int rflag = 0;
  int sflag = 0;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, usage_getoptstr(getattr_usage))) != -1)
    switch (opt)
      {
      case 'r':
        rflag = 1;
        break ;
      case 's':
        sflag = 1;
        break ;
      case '?':
      default:
        usage_help(&getattr_cmd);
      return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  if (1 != argc)
    {
      usage_help(&getattr_cmd);
      return SHELL_CONT;
    }

  path = argv[0];

  if (! strcmp("s3", (char *) dpl_get_backend_name(ctx)))
    {
      if (-1 == path_contains_valid_bucketname(ctx, path))
        {
          fprintf(stderr, "You need to set a bucket to use 'getattr' "
                  "(use 'la' to list the buckets)\n");
          goto end;
        }
    }

  if (1 == rflag)
    {
      ret = dpl_getattr_raw(ctx, path, &metadata);
      if (DPL_SUCCESS != ret)
        {
          fprintf(stderr, "status: %s (%d)\n", dpl_status_str(ret), ret);
          goto end;
        }
      dpl_dict_print(metadata, stdout, 0);
    }
  else
    {
      ret = dpl_getattr(ctx, path, sflag ? NULL : &metadata, sflag ? &sysmd : NULL);
      if (DPL_SUCCESS != ret)
        {
          fprintf(stderr, "status: %s (%d)\n", dpl_status_str(ret), ret);
          goto end;
        }
      if (sflag)
        dpl_sysmd_print(&sysmd, stdout);
      else if (metadata)
        dpl_dict_print(metadata, stdout, 0);
      else
        fprintf(stderr, "No metadata to display.\n");
    }
  
  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  if (NULL != metadata)
    dpl_dict_free(metadata);

  return SHELL_CONT;
}
