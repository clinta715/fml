#ifndef FILEOPS_H
#define FILEOPS_H

#include "fml.h"

int fileops_copy(Panel *src, Panel *dst);
int fileops_move(Panel *src, Panel *dst);
int fileops_delete(Panel *p);
int fileops_mkdir(Panel *p, const char *name);
int fileops_rename(Panel *p, const char *newname);

#endif