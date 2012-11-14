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

#ifndef __CMDS_H__
#define __CMDS_H__	1

extern struct cmd_def cd_cmd;
extern struct cmd_def lcd_cmd;
extern struct cmd_def la_cmd;
extern struct cmd_def put_cmd;
extern struct cmd_def get_cmd;
extern struct cmd_def rm_cmd;
extern struct cmd_def rb_cmd;
extern struct cmd_def getattr_cmd;
extern struct cmd_def mb_cmd;
extern struct cmd_def ls_cmd;
extern struct cmd_def mknod_cmd;
extern struct cmd_def mkdir_cmd;
extern struct cmd_def pwd_cmd;
extern struct cmd_def rmdir_cmd;
extern struct cmd_def set_cmd;
extern struct cmd_def setattr_cmd;
extern struct cmd_def unset_cmd;
extern struct cmd_def genurl_cmd;
extern struct cmd_def cp_cmd;
extern struct cmd_def mv_cmd;
extern struct cmd_def mvdent_cmd;
extern struct cmd_def ln_cmd;
extern struct cmd_def idof_cmd;
extern struct cmd_def mkdent_cmd;
extern struct cmd_def rmdent_cmd;

extern struct cmd_def	*cmd_defs[];

/* PROTO cmds.c cmd_ls.c cmd_cd.c */
/* ./cmds.c */
int cmd_quit(int argc, char **argv);
int cmd_help(int argc, char **argv);
/* ./cmd_ls.c */
dpl_status_t ls_recurse(struct ls_data *ls_data, char *dir, int level);
int cmd_ls(int argc, char **argv);
/* ./cmd_cd.c */
int do_cd(char *path);
int cmd_cd(int argc, char **argv);
#endif
