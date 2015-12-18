#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "../include/processes.h"

int isProcessesEmpty(ProcessNodePtr headPtr) {
    return headPtr == NULL;
}

/* insert a new process to the process queue */
void insertIntoProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr,
                         pid_t process_id, char **args, int number_of_args) {

    /* pointer to a new process */
    ProcessNodePtr newPtr;
    /* loop counter */
    int i;

    /* allocate memory for the new process */
    newPtr = malloc( sizeof(ProcessNode) );

    /* if the memory is available */
    if (newPtr != NULL) {
        newPtr->process_id = process_id;    /* process id of the child */
        newPtr->is_running = RUNNING;       /* it's currently running */
        newPtr->nextProcess = NULL;

        /* store the arguments of the command that created this process */
        newPtr->number_of_args = number_of_args;

        /* allocate enough memory location for the arguments */
        newPtr->args = malloc(sizeof(char*) * (number_of_args));

        /* loop through the arguments */
        for (i = 0; i < number_of_args; i++) {
            /* allocate memory for the current argument */
            newPtr->args[i] = malloc(strlen(args[i]) + 1);

            /* copy the argument to the allocated memory area */
            strcpy(newPtr->args[i], args[i]);
        }

        /* if empty, insert the process at head */
        if (isProcessesEmpty(*headPtr)) {
            /* since it the first process in the list,
             * its index will be 1*/
            newPtr->index = 1;
            *headPtr = newPtr;
        }

        /* otherwise, insert it to the tail */
        else {
            /* since it's not the first process in the list,
             * last index will be incremented by one and assign to new process */
            newPtr->index = (*tailPtr)->index + 1;
            (*tailPtr)->nextProcess = newPtr;
        }

        *tailPtr = newPtr;
    }

    /* if there's not enough memory to create a new node */
    else {
        printf("%ld not inserted. No memory available.\n", (long) process_id);
    }
}

long int removeFromProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr, long int id) {
    ProcessNodePtr previousPtr; /* pointer to the previous node */
    ProcessNodePtr currentPtr;  /* pointer to the current node */
    ProcessNodePtr tempPtr;     /* pointer to the node that will be deleted */

    /* if the process to remove is the first one */
    if (id == (*headPtr)->process_id) {
        /* store the address of the first node in a temporary pointer */
        tempPtr = *headPtr;

        /* assign the next node as the head of the queue */
        *headPtr = (*headPtr)->nextProcess;

        /* free the node */
        free(tempPtr);
        return id;
    }

    /* if the process to remove is not the first one, look for its location */
    else {
        previousPtr = *headPtr;
        currentPtr = (*headPtr)->nextProcess;

        /* loop through the processes to find the correct location */
        while (currentPtr != NULL && currentPtr->process_id != id) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextProcess;
        }

        /* delete node at currentPtr */
        if (currentPtr != NULL) {
            tempPtr = currentPtr;
            previousPtr->nextProcess = currentPtr->nextProcess;
            free(tempPtr);
            return id;
        }
    }

    return 0;
}

/* find the background process by either its index or process id */
ProcessNodePtr findProcess(ProcessNodePtr processes, int id, int id_type) {
    /* if queue is empty, return NULL */
    if (processes == NULL) {
        return NULL;
    }

    /* if the passed parameter is process id */
    if ( id_type == PROCESS_ID ){
        while (processes != NULL) {
            if (processes->process_id == id) return processes;
            processes = processes->nextProcess;
        }
    }

    /* if the passed parameter is index */
    else if (id_type == INDEX) {
        while (processes != NULL) {
            if (processes->index == id) return processes;
            processes = processes->nextProcess;
        }
    }

    /* return NULL if process is not found */
    return NULL;
}