#include "sync.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

/*Initiates the corresponding lock. If NOSYNC, does nothing.*/
void lockInit(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        if (pthread_mutex_init(&mlock, NULL) != 0)
        {
            fprintf(stderr, "Error: failed to initiate mutex lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (sync == RWLOCK)
    {
       if (pthread_rwlock_init(&rwlock, NULL) != 0)
        {
            fprintf(stderr, "Error: failed to initiate rwlock.\n");
            exit(EXIT_FAILURE);
        } 
    }
}

/*Destroys the corresponding lock. If NOSYNC, does nothing.*/
void lockDestroy(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        if (pthread_mutex_destroy(&mlock) != 0)
        {
            fprintf(stderr, "Error: failed to destroy mutex lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (sync == RWLOCK)
    {
       if (pthread_rwlock_destroy(&rwlock) != 0)
        {
            fprintf(stderr, "Error: failed to destroy rwlock.\n");
            exit(EXIT_FAILURE);
        } 
    }
}

/*Locks the corresponding lock (read). If NOSYNC, does nothing.*/
void lockr(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        if (pthread_mutex_lock(&mlock) != 0)
        {
            fprintf(stderr, "Error: failed to lock mutex lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (sync == RWLOCK)
    {
        if (pthread_rwlock_rdlock(&rwlock) != 0)
        {
            fprintf(stderr, "Error: failed to lock rwlock.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/*Locks the corresponding lock (write). If NOSYNC, does nothing.*/
void lockw(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        if (pthread_mutex_lock(&mlock) != 0)
        {
            fprintf(stderr, "Error: failed to lock mutex lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (sync == RWLOCK)
    {
        if (pthread_rwlock_wrlock(&rwlock) != 0)
        {
            fprintf(stderr, "Error: failed to lock rwlock.\n");
            exit(EXIT_FAILURE);
        }
    }
}


/*Unlocks the corresponding lock. If NOSYNC, does nothing.*/
void unlock(syncStrat sync, pthread_mutex_t mlock, pthread_rwlock_t rwlock)
{
    if (sync == MUTEX)
    {
        if (pthread_mutex_unlock(&mlock) != 0)
        {
            fprintf(stderr, "Error: failed to unlock mutex lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    else if (sync == RWLOCK)
    {
        if (pthread_rwlock_unlock(&rwlock) != 0)
        {
            fprintf(stderr, "Error: failed to unlock rwlock.\n");
            exit(EXIT_FAILURE);
        }
    }
}