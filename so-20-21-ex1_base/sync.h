#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>

typedef enum syncStrat { MUTEX, RWLOCK, NOSYNC } syncStrat;

void lockr(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);
void lockw(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);
void unlock(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock);

#endif