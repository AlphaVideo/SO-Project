#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

/* Global Locks */
extern pthread_mutex_t commandlock;

void lockrd(pthread_rwlock_t *lock);
void lockwr(pthread_rwlock_t *lock);
void unlock(pthread_rwlock_t *lock);

/* removeCommands needs it's own lock */
void commandLockLock();
void commandLockUnlock();
void destroyLocks();

/* DEBUG */
void printLockList(pthread_rwlock_t **lockList);

#endif