#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include "../sync.h"
#include "../tecnicofs-api-constants.h"

/* Locks globais */
extern pthread_mutex_t mlock;
extern pthread_rwlock_t rwlock;

/* FS root inode number */
#define FS_ROOT 0

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES 20

#define SUCCESS 0
#define FAIL -1

#define DELAY 5000


/*
 * Contains the name of the entry and respective i-number
 */
typedef struct dirEntry {
	char name[MAX_FILE_NAME];
	int inumber;
} DirEntry;

/*
 * Data is either text (file) or entries (DirEntry)
 */
union Data {
	char *fileContents; /* for files */
	DirEntry *dirEntries; /* for directories */
};

/*
 * I-node definition
 */
typedef struct inode_t {    
	type nodeType;
	union Data data;
    /* more i-node attributes will be added in future exercises */
} inode_t;


void insert_delay(int cycles);
void inode_table_init();
void inode_table_destroy();
int inode_create(type nType, syncStrat sync);
int inode_delete(int inumber, syncStrat sync);
int inode_get(int inumber, type *nType, union Data *data, syncStrat sync);
int inode_set_file(int inumber, char *fileContents, int len);
int dir_reset_entry(int inumber, int sub_inumber, syncStrat sync);
int dir_add_entry(int inumber, int sub_inumber, char *sub_name, syncStrat sync);
void inode_print_tree(FILE *fp, int inumber, char *name);


#endif /* INODES_H */
