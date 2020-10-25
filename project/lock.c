#include "lock.h"
#include <stdio.h>
#include <stdlib.h>



/*Read locks the given node lock.*/
void lockrd(pthread_rwlock_t *lock)
{

    if (pthread_rwlock_rdlock(lock) != 0)
    {
        fprintf(stderr, "Error: failed to lock (read) rwlock.\n");
        exit(EXIT_FAILURE);
    }
}

/*Write locks the given node lock.*/
void lockwr(pthread_rwlock_t *lock)
{
    if (pthread_rwlock_wrlock(lock) != 0)
    {
        fprintf(stderr, "Error: failed to lock (write) rwlock.\n");
        exit(EXIT_FAILURE);
    }
}


/*Unlocks the given node lock.*/
void unlock(pthread_rwlock_t *lock)
{
    if (pthread_rwlock_unlock(lock) != 0)
    {
        fprintf(stderr, "Error: failed to unlock rwlock.\n");
        exit(EXIT_FAILURE);
    }
}


/* Locks command lock.*/
void commandLockLock()
{
    if (pthread_mutex_lock(&commandlock) != 0)
    {
        fprintf(stderr, "Error: failed to lock command lock.\n");
        exit(EXIT_FAILURE);
    }

}

/* Unlocks command lock.*/
void commandLockUnlock()
{
    if (pthread_mutex_unlock(&commandlock) != 0)
    {
        fprintf(stderr, "Error: failed to unlock command lock.\n");
        exit(EXIT_FAILURE);
    }
}

/* Destroys all global locks initialized in main. */
void destroyLocks()
{
    if (pthread_mutex_destroy(&commandlock) != 0)
    {
        fprintf(stderr, "Error: couldn't destroy command lock.\n");
        exit(EXIT_FAILURE);
    }
}


void printLockList(pthread_rwlock_t **lockList)
{
    int i;
    for(i = 0; i < 50; i++)
    {
        if(lockList[i] == NULL)
        {
            printf("#%d| 0 |\n", i);
        }
        else
        {
            printf("#%d| 1 |\n", i);
        }
    }
}