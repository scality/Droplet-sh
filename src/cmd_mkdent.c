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

int cmd_mkdent(int argc, char **argv);

struct usage_def mkdent_usage[] =
  {
    {USAGE_NO_OPT, USAGE_MANDAT, "id", ""},
    {USAGE_NO_OPT, USAGE_MANDAT, "path", ""},
    {'t', USAGE_PARAM, "type", "object type"},
    {'l', 0u, NULL, "list object types"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def mkdent_cmd = {"mkdent", "create dentry", mkdent_usage, cmd_mkdent};

int
cmd_mkdent(int argc,
          char **argv)
{
  char opt;
  int ret;
  char *id = NULL;
  char *path = NULL;
  dpl_ftype_t ftype = DPL_FTYPE_REG;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, "t:l")) != -1)
    switch (opt)
      {
      case 'l':
        list_object_types();
        return SHELL_CONT;
        break ;
      case 't':
        if (!strcasecmp(optarg, "reg"))
          ftype = DPL_FTYPE_REG;
        else if (!strcasecmp(optarg, "chrdev"))
          ftype = DPL_FTYPE_CHRDEV;
        else if (!strcasecmp(optarg, "blkdev"))
          ftype = DPL_FTYPE_BLKDEV;
        else if (!strcasecmp(optarg, "fifo"))
          ftype = DPL_FTYPE_FIFO;
        else if (!strcasecmp(optarg, "socket"))
          ftype = DPL_FTYPE_SOCKET;
        else if (!strcasecmp(optarg, "symlink"))
          ftype = DPL_FTYPE_SYMLINK;
        else
          {
            fprintf(stderr, "not supported\n");
            return SHELL_CONT;
          }
        break ;
      case '?':
      default:
        usage_help(&mkdent_cmd);
        return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  if (2 != argc)
    {
      usage_help(&mkdent_cmd);
      return SHELL_CONT;
    }

  id = argv[0];
  path = argv[1];

  ret = dpl_mkdent(ctx, id, path, ftype);
  if (DPL_SUCCESS != ret)
    {
      fprintf(stderr, "status %s (%d)\n", dpl_status_str(ret), ret);
      goto end;
    }

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  return SHELL_CONT;
}
