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

int cmd_ls(int argc, char **argv);

struct usage_def ls_usage[] =
  {
    {'l', 0u, NULL, "long display"},
    {'d', 0u, NULL, "getattr on directory"},
    {'a', 0u, NULL, "ignored for now"},
    {'R', 0u, NULL, "recurse subdirectories"},
    {'A', 0u, NULL, "list all files in the bucket (do not use vdir interface)"},
    {USAGE_NO_OPT, 0u, "path or bucket", "remote directory"},
    {0, 0u, NULL, NULL},
  };

struct cmd_def ls_cmd = {"ls", "list directory", ls_usage, cmd_ls};

dpl_status_t
ls_recurse(struct ls_data *ls_data,
           char *path,
           int level)
{
  int ret;

  if (1 == ls_data->Xflag)
    {
      dpl_vec_t *objects = NULL;
      int i;

      //raw listing
      ret = dpl_list_bucket(ctx, ctx->cur_bucket, NULL, NULL, -1, &objects, NULL);
      if (DPL_SUCCESS != ret)
        {
          fprintf(stderr, "listbucket failure %s (%d)\n", dpl_status_str(ret), ret);
          return ret;
        }

      for (i = 0;i < objects->n_items;i++)
        {
          dpl_object_t *obj = (dpl_object_t *) dpl_vec_get(objects, i);

          if (0 == ls_data->pflag)
            {
              if (ls_data->lflag)
                {
                  struct tm *stm;

                  stm = localtime(&obj->last_modified);
                  printf("%12llu %04d-%02d-%02d %02d:%02d %s\n", (unsigned long long) obj->size, 1900 + stm->tm_year, 1 + stm->tm_mon, stm->tm_mday, stm->tm_hour, stm->tm_min, obj->path);
                }
              else
                {
                  printf("%s\n", obj->path);
                }
            }

          ls_data->total_size += obj->size;
        }

      if (NULL != objects)
        dpl_vec_objects_free(objects);
    }
  else
    {
      void *dir_hdl;
      dpl_dirent_t entry;
      dpl_fqn_t cur_fqn;
      struct tm *stm = NULL;
      dpl_sysmd_t sysmd;

      memset(&sysmd, 0, sizeof sysmd);

      ret = dpl_getattr(ctx, path, NULL, &sysmd);
      if (DPL_SUCCESS != ret)
        {
          printf("dpl_getattr(%s): %s (%d)\n", path, dpl_status_str(ret), ret);
          return ret;
        }

      if (DPL_FTYPE_DIR != sysmd.ftype ||
          ls_data->dflag) /* -d supersedes -R */
        {
          if (ls_data->lflag)
            {
              stm = localtime(&sysmd.mtime);
              printf("%12llu %04d-%02d-%02d %02d:%02d %s\n",
                     (unsigned long long) sysmd.size,
                     1900 + stm->tm_year,
                     1 + stm->tm_mon,
                     stm->tm_mday,
                     stm->tm_hour,
                     stm->tm_min,
                     path);
            }
          else
            {
              printf("%s\n", path);
            }
        }
      else
        {
          if (1 == ls_data->Rflag)
            {
              ret = dpl_chdir(ctx, path);
              if (DPL_SUCCESS != ret)
                return ret;

              cur_fqn = dpl_cwd(ctx, ctx->cur_bucket);

              printf("%s/%s:\n", 0 == level ? "" : "\n", cur_fqn.path);

              ret = dpl_opendir(ctx, ".", &dir_hdl);
              if (DPL_SUCCESS != ret)
                return ret;
            }
          else
            {
              ret = dpl_opendir(ctx, path, &dir_hdl);
              if (DPL_SUCCESS != ret)
                return ret;
            }

          while (!dpl_eof(dir_hdl))
            {
              ret = dpl_readdir(dir_hdl, &entry);
              if (DPL_SUCCESS != ret)
                return ret;

              if (0 == ls_data->pflag)
                {
                  if (ls_data->lflag)
                    {
                      memset(&sysmd, 0, sizeof (sysmd));

                      if (!strcmp((char *) dpl_get_backend_name(ctx), "s3"))
                        {
                          //optim for S3
                          sysmd.mtime = entry.last_modified;
                          sysmd.size = entry.size;
                        }
                      else
                        {
                          char entrypath[1024];
                          (void) sprintf(entrypath, "%s/%s", path, entry.name);
                          ret = dpl_getattr(ctx, entrypath, NULL, &sysmd);
                          if (DPL_SUCCESS != ret)
                            fprintf(stderr,
                                    "dpl_getattr(%s) failed: %s (%d)\n",
                                    entry.name, dpl_status_str(ret), ret);
                        }

                      stm = localtime(&sysmd.mtime);
                      printf("%12llu %04d-%02d-%02d %02d:%02d %s\n",
                             (unsigned long long) sysmd.size,
                             1900 + stm->tm_year,
                             1 + stm->tm_mon,
                             stm->tm_mday,
                             stm->tm_hour,
                             stm->tm_min,
                             entry.name);
                    }
                  else
                    {
                      printf("%s\n", entry.name);
                    }
                }

              ls_data->total_size += entry.size;

              if (1 == ls_data->Rflag &&
                  strcmp(entry.name, ".") &&
                  (DPL_FTYPE_DIR == entry.type))
                {
                  ret = ls_recurse(ls_data, entry.name, level + 1);
                  if (DPL_SUCCESS != ret)
                    return ret;
                }
            }

          dpl_closedir(dir_hdl);

          if (1 == ls_data->Rflag && level > 0)
            {
              ret = dpl_chdir(ctx, "..");
              if (DPL_SUCCESS != ret)
                return ret;
            }
        }
    }

  return DPL_SUCCESS;
}

int
cmd_ls(int argc,
       char **argv)
{
  char opt;
  int ret;
  int lflag = 0;
  int Xflag = 0;
  int Rflag = 0;
  int dflag = 0;
  size_t total_size = 0;
  char *path;
  struct ls_data ls_data;
  int i;

  var_set("status", "1", VAR_CMD_SET, NULL);

  optind = 0;

  while ((opt = linux_getopt(argc, argv, usage_getoptstr(ls_usage))) != -1)
    switch (opt)
      {
      case 'd':
        dflag = 1;
        break;
      case 'R':
        Rflag = 1;
        break ;
      case 'a':
        break ;
      case 'A':
        Xflag = 1;
        break ;
      case 'l':
        lflag = 1;
        break ;
      case '?':
      default:
        usage_help(&ls_cmd);
        return SHELL_CONT;
      }
  argc -= optind;
  argv += optind;

  memset(&ls_data, 0, sizeof (ls_data));
  ls_data.ctx = ctx;
  ls_data.lflag = lflag;
  ls_data.Rflag = Rflag;
  ls_data.Xflag = Xflag;
  ls_data.dflag = dflag;

  if (! strcmp("s3", (char *) dpl_get_backend_name(ctx)))
    {
      /* we can do 'ls bucket:/path' or 'ls bucket:', even if no bucket is set */
      if (argv[0])
        {
          char *p = strchr(argv[0], ':');
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

              if (! strcmp(argv[0], (char *) item->string))
                  found = 1;
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
              fprintf(stderr, "You need to set a bucket to use 'ls', "
                      "or use 'la' to list the buckets\n");
              goto end;
            }
        }
    }

  if (0 == argc)
    {
      ret = ls_recurse(&ls_data, ".", 0);
      if (DPL_SUCCESS != ret)
        {
          fprintf(stderr, "ls failure %s (%d)\n", dpl_status_str(ret), ret);
          goto end;
        }
    }
  else
    {
      for (i = 0; i < argc; i++)
        {
          ret = ls_recurse(&ls_data, argv[i], 0);
          if (DPL_SUCCESS != ret)
            {
              fprintf(stderr, "ls failure %s (%d)\n", dpl_status_str(ret), ret);
              goto end;
            }
        }
    }

  total_size = ls_data.total_size;

  if (1 == lflag)
    {
      if (NULL != ctx->pricing)
        printf("Total %s Price %s\n", dpl_size_str(total_size), dpl_price_storage_str(ctx, total_size));
    }

  var_set("status", "0", VAR_CMD_SET, NULL);

 end:

  return SHELL_CONT;
}
