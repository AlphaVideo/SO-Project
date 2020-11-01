#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

/* Global Locks */
extern pthread_mutex_t commandlock;
extern pthread_cond_t canInsert;
extern pthread_cond_t canRemove;

void lockrd(pthread_rwlock_t *lock);
void lockwr(pthread_rwlock_t *lock);
void unlock(pthread_rwlock_t *lock);

/* removeCommands needs it's own lock */
void commandLockLock();
void commandLockUnlock();
void destroyGlobalLock();

/* DEBUG */
void printLockList(pthread_rwlock_t **lockList);

#endif