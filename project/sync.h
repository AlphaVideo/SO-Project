#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

typedef enum syncStrat { MUTEX, RWLOCK, NOSYNC } syncStrat;

/* Global Locks */
extern pthread_mutex_t commandlock;

void lockrd(pthread_rwlock_t *lock);
void lockwr(pthread_rwlock_t *lock);
void unlock(pthread_rwlock_t *lock);


/* removeCommands needs it's own lock */
void commandLockLock();
void commandLockUnlock();

void destroyLocks();

#endif