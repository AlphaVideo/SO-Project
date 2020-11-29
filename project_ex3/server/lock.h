#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

extern pthread_mutex_t printMutex;
extern pthread_cond_t printCond;

void lockrd(pthread_rwlock_t *lock);
void lockwr(pthread_rwlock_t *lock);
void unlock(pthread_rwlock_t *lock);
void printLock();
void printUnlock();

/* DEBUG */
void printLockList(pthread_rwlock_t **lockList);

#endif