#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

typedef enum syncStrat { MUTEX, RWLOCK, NOSYNC } syncStrat;

void lockr(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);
void lockw(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);
void unlock(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);

#endif