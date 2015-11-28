#define PIPE "|"

/* Struct to construct a list of paths. */
struct command_node {
    int index;
    char **args;
    int count;
    char *next_symbol;
    struct command_node *nextCommand;
};

typedef struct command_node CommandNode; /* synonym for struct command_node */
typedef CommandNode *CommandNodePtr; /* synonym for CommandNode* */

CommandNodePtr headCommand = NULL; /* initialize headPtr */
CommandNodePtr tailCommand = NULL; /* initialize tailPtr */

int isCommandEmpty(CommandNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char **args, char *symbol, int count ) {
    int i;
    CommandNodePtr newPtr = NULL; /* pointer to a new command */

    newPtr = malloc( sizeof(CommandNode) );

    if (newPtr != NULL) {
        newPtr->count = count;

        /* allocate memory for each argument */
        newPtr->args = malloc(sizeof(char*) * (count + 1));
        for (i = 0; i < count; i++) {
            newPtr->args[i] = malloc(strlen(args[i]) + 1);
            strcpy(newPtr->args[i], args[i]);
        }

        /* The array of pointers must be terminated by a NULL pointer. */
        newPtr->args[i] = NULL;
        newPtr->next_symbol = symbol;

        /* if empty, insert node at head */
        if (isCommandEmpty(*headPtr)) {
            newPtr->index = 0;
            *headPtr = newPtr;
        } else {
            newPtr->index = (*tailPtr)->index + 1;
            (*tailPtr)->nextCommand = newPtr;
        }

        newPtr->nextCommand = NULL;
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
        for (i = 0; i <= temp->count; ++i) {
            free(temp->args[i]);
        }

        free(temp->args);
        free(temp);

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
                fprintf(stderr, "currentCommand->next_symbol: NULL\n");
            else
                fprintf(stderr, "currentCommand->next_symbol: %s\n\n", currentCommand->next_symbol);

            currentCommand = currentCommand->nextCommand;
        }

    }
}