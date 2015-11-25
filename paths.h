/* Struct to construct a list of paths. */
struct path_node {
    char *path;
    struct path_node *nextPath;
};

typedef struct path_node PathNode; /* synonym for struct path_node */
typedef PathNode *PathNodePtr; /* synonym for PathNode* */

PathNodePtr headPaths = NULL; /* initialize headPtr */
PathNodePtr tailPaths = NULL; /* initialize tailPtr */

int isEmpty(PathNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoPath( PathNodePtr *headPtr, PathNodePtr *tailPtr, char *value ) {
    PathNodePtr newPtr; /* pointer to a new path */
    newPtr = malloc( sizeof(PathNode) );

    if (newPtr != NULL) {
        newPtr->path = value;
        newPtr->nextPath = NULL;

        /* if empty, insert node at head */
        if (isEmpty(*headPtr)) {
            *headPtr = newPtr;
        } else {
            (*tailPtr)->nextPath = newPtr;
        }

        *tailPtr = newPtr;
    } else {
        printf( "%s not inserted. No memory available.\n", value );
    }

}

void printPaths(PathNodePtr currentPath) {
    /* if queue is empty */
    if (currentPath == NULL) {
        printf("Path list is empty");
    }

    else {
        while ( currentPath != NULL ) {
            printf("%s", currentPath->path);
            printf("\n");
            currentPath = currentPath->nextPath;
        }

    }
}