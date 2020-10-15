#include "sync.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


/*Locks the corresponding lock (read). If NOSYNC, does nothing.*/
void lockr(syncStrat sync)
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
void lockw(syncStrat sync)
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
void unlock(syncStrat sync)
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


/* Locks command lock. Does nothing if NOYSNC.*/
void commandLockLock(syncStrat sync)
{
    if (sync != NOSYNC)
    {
        if (pthread_mutex_lock(&commandlock) != 0)
        {
            fprintf(stderr, "Error: failed to lock command lock.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/* Unlocks command lock. Does nothing if NOYSNC.*/
void commandLockUnlock(syncStrat sync)
{
    if (sync != NOSYNC)
    {
        if (pthread_mutex_unlock(&commandlock) != 0)
        {
            fprintf(stderr, "Error: failed to unlock command lock.\n");
            exit(EXIT_FAILURE);
        }
    }
}

/* Destroys all locks initialized in main. */
void destroyLocks()
{
    if (pthread_mutex_destroy(&commandlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy command lock.\n");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_mutex_destroy(&mlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy mutex lock.\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_rwlock_destroy(&rwlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy rwlock.\n");
        exit(EXIT_FAILURE);
    }
}