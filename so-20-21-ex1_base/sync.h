#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

typedef enum syncStrat { MUTEX, RWLOCK, NOSYNC } syncStrat;

/* Global Locks */
extern pthread_mutex_t commandlock;
extern pthread_mutex_t mlock;
extern pthread_rwlock_t rwlock;


void lockInit(syncStrat sync);
void lockDestroy(syncStrat sync);
void lockr(syncStrat sync);
void lockw(syncStrat sync);
void unlock(syncStrat sync);

void commandLockInit(syncStrat sync);
void commandLockDestroy(syncStrat sync);
void commandLockLock(syncStrat sync);
void commandLockUnlock(syncStrat sync);

#endif