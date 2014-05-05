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

int cmd_cd(int argc, char **argv);

struct usage_def cd_usage[] =
  {
    {USAGE_NO_OPT, USAGE_MANDAT, "path", "remote path"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def cd_cmd = {"cd", "change directory", cd_usage, cmd_cd};

char *path_saved = NULL;

int
do_cd(char *path)
{
  int ret;
  char wd[1024];
  dpl_fqn_t cur_fqn;

  cur_fqn = dpl_cwd(ctx, ctx->cur_bucket);

  snprintf(wd, sizeof (wd), "%s:/%s", ctx->cur_bucket, cur_fqn.path);

  ret = dpl_chdir(ctx, path);
  if (DPL_SUCCESS != ret)
    {
      fprintf(stderr, "chdir failed %s: %s (%d)\n", path, dpl_status_str(ret), ret);
      return DPL_FAILURE;
    }

  if (NULL != path_saved)
    free(path_saved);
  path_saved = strdup(wd);

  return DPL_SUCCESS;
}

int
cmd_cd(int argc,
       char **argv)
{
  int ret;
  char *path;

  var_set("status", "1", VAR_CMD_SET, NULL);

  if (2 != argc)
    {
      usage_help(&cd_cmd);
      return SHELL_CONT;
    }

  path = argv[1];

  if (! strcmp("s3", (char *) dpl_get_backend_name(ctx)))
    {
      if (-1 == path_contains_valid_bucketname(ctx, argv[1]))
        {
          fprintf(stderr, "You need to set a bucket to use 'cd' "
                  "(use 'la' to list the buckets)\n");
          goto end;
        }
    }

  if (!strcmp(path, "-") && NULL != path_saved)
    path = path_saved;

  ret = do_cd(path);
  if (DPL_SUCCESS != ret)
    {
      goto end;
    }

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  return SHELL_CONT;
}
