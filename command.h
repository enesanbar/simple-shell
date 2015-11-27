/* Struct to construct a list of paths. */
struct command_node {
    char *args;
    char *next_symbol;
    struct command_node *nextCommand;
};

typedef struct command_node CommandNode; /* synonym for struct command_node */
typedef CommandNode *CommandNodePtr; /* synonym for CommandNode* */

CommandNodePtr headPaths = NULL; /* initialize headPtr */
CommandNodePtr tailPaths = NULL; /* initialize tailPtr */

int isEmpty(CommandNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char *args[], char *symbol ) {
    CommandNodePtr newPtr; /* pointer to a new path */
    newPtr = malloc( sizeof(CommandNode) );

    if (newPtr != NULL) {
        newPtr->args = args;
        newPtr->nextCommand = NULL;

        if (symbol != NULL)
            newPtr->next_symbol = symbol;

        /* if empty, insert node at head */
        if (isEmpty(*headPtr)) {
            *headPtr = newPtr;
        } else {
            (*tailPtr)->nextCommand = newPtr;
        }

        *tailPtr = newPtr;
    } else {
        printf( "%s not inserted. No memory available.\n", args );
    }

}

void printCommands(CommandNodePtr currentPath) {
    /* if queue is empty */
    if (currentPath == NULL) {
        printf("Path list is empty");
    }

    else {
        while ( currentPath != NULL ) {
;
        }

    }
}