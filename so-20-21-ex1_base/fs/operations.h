#ifndef FS_H
#define FS_H
#include "state.h"
#include "../sync.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries, syncStrat sync);
int create(char *name, type nodeType, syncStrat sync);
int delete(char *name, syncStrat sync);
int lookup(char *name, syncStrat sync);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
