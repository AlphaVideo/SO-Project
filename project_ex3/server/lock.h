#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

void lockrd(pthread_rwlock_t *lock);
void lockwr(pthread_rwlock_t *lock);
void unlock(pthread_rwlock_t *lock);

/* DEBUG */
void printLockList(pthread_rwlock_t **lockList);

#endif