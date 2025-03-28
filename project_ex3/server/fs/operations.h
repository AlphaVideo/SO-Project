#ifndef FS_H
#define FS_H
#include "state.h"
#include "../lock.h"

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int move(char *origPath, char *destPath);
int lookup(char *name, pthread_rwlock_t **lookupLocks);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
