/* Struct to construct a list of paths. */
struct command_node {
    char *next_symbol;
    struct command_node *nextCommand;
    int count;
    char *args[];
};

typedef struct command_node CommandNode; /* synonym for struct command_node */
typedef CommandNode *CommandNodePtr; /* synonym for CommandNode* */

CommandNodePtr headCommand = NULL; /* initialize headPtr */
CommandNodePtr tailCommand = NULL; /* initialize tailPtr */

int isCommandEmpty(CommandNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char *args[], char *symbol, int count ) {
    CommandNodePtr newPtr; /* pointer to a new path */
    newPtr = malloc( sizeof(CommandNode) );

    if (newPtr != NULL) {
        int i;
        for (i = 0; i < count; i++) {
            newPtr->args[i] = malloc(sizeof(args[i]));
            newPtr->args[i] = args[i];
        }

        newPtr->count = count;
        newPtr->nextCommand = NULL;

        if (symbol != NULL)
            newPtr->next_symbol = symbol;

        /* if empty, insert node at head */
        if (isCommandEmpty(*headPtr)) {
            *headPtr = newPtr;
        } else {
            (*tailPtr)->nextCommand = newPtr;
        }

        *tailPtr = newPtr;
    } else {
        printf( "command not inserted. No memory available.\n");
    }
}

void emptyCommands(CommandNodePtr *headPtr, CommandNodePtr *tailPtr) {
    CommandNodePtr temp;

    while (*headPtr != NULL) {

        temp = *headPtr;
        *headPtr = (*headPtr)->nextCommand;

        /* if queue is empty */
        if (*headPtr == NULL) {
            *tailPtr = NULL;
        }

        int i;
        for (i = 0; i < temp->count; ++i) {
            free(temp->args[i]);
        }

    }
}

void printCommands(CommandNodePtr currentCommand) {
    /* if queue is empty */
    if (currentCommand == NULL) {
        printf("Command list is empty");
    }

    else {
        while ( currentCommand != NULL ) {
            int i;
            for (i = 0; i < currentCommand->count; i++)
                fprintf(stderr, "args %d = %s\n", i, currentCommand->args[i]);

            if (currentCommand->nextCommand == NULL)
                fprintf(stderr, "next_symbol: NULL\n");
            else
                fprintf(stderr, "currentCommand->next_symbol: %s\n\n", currentCommand->next_symbol);

            currentCommand = currentCommand->nextCommand;
        }

    }
}