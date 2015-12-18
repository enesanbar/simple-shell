#include <stdio.h>
#include <stdlib.h>
#include "../include/paths.h"


/* insert a new path to the path queue */
void insertIntoPath( PathNodePtr *headPtr, PathNodePtr *tailPtr, char *path ) {
    /* pointer to a new path */
    PathNodePtr newPtr;

    /* allocate memory for the new node */
    newPtr = malloc( sizeof(PathNode) );

    /* if the memory is available */
    if (newPtr != NULL) {
        /* assign the path to struct */
        newPtr->path = path;
        newPtr->nextPath = NULL;

        /* if empty, insert the new node at the head */
        if (*headPtr == NULL) {
            *headPtr = newPtr;
        }
        /* otherwise, insert it at the tail */
        else {
            (*tailPtr)->nextPath = newPtr;
        }

        *tailPtr = newPtr;
    }
    /* if there's not enough memory available */
    else {
        printf( "%s not inserted. No memory available.\n", path );
    }

}