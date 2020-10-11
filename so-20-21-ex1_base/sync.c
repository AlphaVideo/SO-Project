#include "sync.h"
#include <pthread.h>
#include <stdio.h>

/*Locks the corresponding lock (read). If NOSYNC, does nothing.*/
void lockr(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        pthread_mutex_lock(&mlock);
    }
    else if (sync == RWLOCK)
    {
        pthread_rwlock_rdlock(&rwlock);
    }
}

/*Locks the corresponding lock (write). If NOSYNC, does nothing.*/
void lockw(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        pthread_mutex_lock(&mlock);
    }
    else if (sync == RWLOCK)
    {
        pthread_rwlock_wrlock(&rwlock);
    }
}


/*Unlocks the corresponding lock. If NOSYNC, does nothing.*/
void unlock(int sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        pthread_mutex_unlock(&mlock);
    }
    else if (sync == RWLOCK)
    {
        pthread_rwlock_unlock(&rwlock);
    }
}