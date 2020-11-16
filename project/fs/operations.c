#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;
}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */
int is_dir_empty(DirEntry *dirEntries) {
	if (dirEntries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}
	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {

	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	pthread_rwlock_t *lockList[INODE_TABLE_SIZE] = {NULL};

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);
	
	split_parent_child_from_path(name_copy, &parent_name, &child_name);
	
	parent_inumber = lookup(parent_name, lockList);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n",
		        name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	/* Parent is already locked in read mode due to the lookup */
	lockListSwitchToWr(parent_inumber, lockList);
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n",
		        name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n",
		       child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	/* create node and add entry to folder that contains new node */
	child_inumber = inode_create(nodeType);
	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n",
		        child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n",
		       child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	lockListClear(lockList);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	pthread_rwlock_t *lockList[INODE_TABLE_SIZE] = {NULL};

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);
	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	parent_inumber = lookup(parent_name, lockList);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n",
		        child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	/* Parent is already locked in read mode due to the lookup */
	lockListSwitchToWr(parent_inumber, lockList);
	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n",
		        child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n",
		       name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	lockListAddWr(child_inumber, lockList);
	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n",
		       name);
		lockListClear(lockList);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n",
		       child_name, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n",
		       child_inumber, parent_name);
		lockListClear(lockList);
		return FAIL;
	}

	lockListClear(lockList);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name, pthread_rwlock_t **lookupLocks){

	char full_path[MAX_FILE_NAME];
	char delim[] = "/";
	char *saveptr;

	strcpy(full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	lockListAddRd(current_inumber, lookupLocks);
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim, &saveptr); 

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		lockListAddRd(current_inumber, lookupLocks);
		inode_get(current_inumber, &nType, &data);
		path = strtok_r(NULL, delim, &saveptr); 
	}
	return current_inumber;
}

/*
 * Move file or directory to a new path.
 * Input:
 *  - origPath: starting path (already verified that exists)
 *  - destPath: destination path (must be in an existent directory, but must not exist)
 * Returns: SUCCESS or FAIL
 */
int move(char *origPath, char *destPath)
{
	/*USAR SO UMA LISTA DE TRINCOS
	USAR INODE_GET
	SO E PRECISO DE DOIS LOOKUPS
	m /a/b c
	*/

	pthread_rwlock_t *lockList[INODE_TABLE_SIZE] = {NULL};

	/* Destination parameters */
	int destParentInumber, destination_inumber;
	char *destParentName, *destChildName;
	type destParentType;
	union Data destParentData;

	/* Origin parameters */
	int origParentInumber, origin_inumber;
	char *origParentName, *origChildName;

	/* Split parent_child_from _path will alter paths*/
	char destPathCopy[MAX_PATH_SIZE], origPathCopy[MAX_PATH_SIZE];
	strcpy(destPathCopy, destPath);
	strcpy(origPathCopy, origPath);

	split_parent_child_from_path(destPathCopy, &destParentName, &destChildName);
	split_parent_child_from_path(origPathCopy, &origParentName, &origChildName);

	/* Destination path's parent directory must exist, but child must not exist */

	destParentInumber = lookup(destParentName, lockList);

	/* Destination parent directory must exist */
	if (destParentInumber == FAIL) 
	{
		printf("failed to move %s to %s, invalid destination parent dir %s\n", origPath, destPath, destParentName);
		lockListClear(lockList);
		return FAIL;
	}

	/* Destination can't already exist */
	inode_get(destParentInumber, &destParentType, &destParentData);
	destination_inumber = lookup_sub_node(destChildName, destParentData.dirEntries);

	if(destination_inumber != FAIL)
	{
		printf("failed to move %s to %s, destination path %s already exists\n", origPath, destPath, destChildName);
		lockListClear(lockList);
		return FAIL;
	}

	/* Can't move directory into itself */
	origin_inumber = lookup(origPath, originLocks1);
	if(origin_inumber == destParentInumber)
	{
		printf("failed to move %s to %s, can't move directory into itself\n", origPath, destPath);
		lockListClear(destLocks2);
		lockListClear(originLocks1);
		return FAIL;
	}

	origParentInumber = lookup(origParentName, originLocks2);
	lockListClear(originLocks1); /* No longer necessary */

	moveMergeLocks(destLocks2, originLocks2);
	
	/* The order of the operations is based on the inumbers of the origin and destination */
	if(destParentInumber > origin_inumber)
	{
		/* Move with new name */
		lockListSwitchToWr(destParentInumber, destLocks2);
		dir_add_entry(destParentInumber, origin_inumber, destChildName);

		/* Delete original */
		lockListSwitchToWr(origParentInumber, originLocks2);
		dir_reset_entry(origParentInumber, origin_inumber);

	}
	else
	{
		/* Delete original */
		lockListSwitchToWr(origParentInumber, originLocks2);
		dir_reset_entry(origParentInumber, origin_inumber);

		/* Move with new name */
		lockListSwitchToWr(destParentInumber, destLocks2);
		dir_add_entry(destParentInumber, origin_inumber, destChildName);
	}

	lockListClear(originLocks2);
	lockListClear(destLocks2);
	return SUCCESS;
}

/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}
