#include "err_check.h"

#include <stdio.h>      // puts
#include <pthread.h>    // pthread_*
#include <stdlib.h>     // exit
#include <errno.h>
#include <semaphore.h>

#define DEFAULT_ATTR NULL
#define NO_ARG NULL
#define NO_STATUS NULL
#define IGNORE_STATUS NULL
#define NOT_PSHARED

static const int COUNT_FROM = 1;
static const int COUNT_TO = 10;
static const int NAME_LENGTH = 8;
static const int SUCCESS = 0;

#define SEMAPHORE_NUMBER 2
static sem_t semaphores[SEMAPHORE_NUMBER];

enum Entity {
    CHILD = 0,
    PARENT = 1,
};

void
PrintCount (enum Entity executingEntity, const char *name, int from, int to) {
    int count;
    const enum Entity waitingEntity = executingEntity == PARENT ? CHILD : PARENT;

    for (count = from; count <= to; ++count) {
        (void) sem_wait (&semaphores[executingEntity]);

        (void) printf ("%*s counts %d\n", NAME_LENGTH, name, count);

        (void) sem_post (&semaphores[waitingEntity]);
    }
}

void*
RunChild (void *ignored) {
    PrintCount (CHILD, "Child", COUNT_FROM, COUNT_TO);
    pthread_exit (NO_STATUS);
}

int
InitializeResources () {
    int code = SUCCESS;

    for (int i = 0; i < SEMAPHORE_NUMBER; ++i) {
#ifndef __APPLE__
        code = sem_init (&semaphores[i], NOT_PSHARED, i);
#endif
        if (code == ENOSPC) {
            (void) fprintf (stderr, "A resource required to initialize the semaphore has been exhausted, "
                    "or the limit on semaphores ({SEM_NSEMS_MAX}) has been reached.");
            return code;
        }
    }

    return SUCCESS;
}

int
DestroyResources () {
    for (int i = 0; i < SEMAPHORE_NUMBER; ++i) {
#ifndef __APPLE__
        (void) sem_destroy (&semaphores[i]);
#endif
    }

    return SUCCESS;
}

int
main (int argc, char **argv) {
    pthread_t child_thread;
    int exit_status = EXIT_SUCCESS;
    int code;
    code = InitializeResources ();
    ExitIfNonZeroWithMessage (code, "Couldn't initialize resources");

    code = pthread_create (&child_thread, DEFAULT_ATTR, RunChild, NO_ARG);
    if (code != SUCCESS) {
        (void) DestroyResources ();
        (void) fputs ("Couldn't start child_thread\n", stderr);
        exit (EXIT_FAILURE);
    };

    PrintCount (PARENT, "Parent", COUNT_FROM, COUNT_TO);

    (void) pthread_join (child_thread, IGNORE_STATUS);

    code = DestroyResources ();
    if (code != SUCCESS) {
        exit_status = EXIT_FAILURE;
        (void) fputs ("Error in DestroyResources\n", stderr);
    };

    exit (exit_status);
}