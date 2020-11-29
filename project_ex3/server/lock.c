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

/* Locks lock for printing operation.*/
void printLock()
{
    if (pthread_mutex_lock(&printMutex) != 0)
    {
        fprintf(stderr, "Error: failed to lock print lock.\n");
        exit(EXIT_FAILURE);
    }

}

/* Unlocks lock for printing operation.*/
void printUnlock()
{
    if (pthread_mutex_unlock(&printMutex) != 0)
    {
        fprintf(stderr, "Error: failed to unlock print lock.\n");
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