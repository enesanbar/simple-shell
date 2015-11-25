struct command_node {
    char name[256];			    /* User printable name of the function. */
    char path[512];             /* The path of the system command */
    struct command_node *nextCommand;
};

typedef struct command_node CommandNode; /* synonym for struct process_node */
typedef CommandNode *CommandNodePtr; /* synonym for CommandNode* */

CommandNodePtr commands = NULL; /* initialize commands */

void insertCommand (CommandNodePtr *sPtr, char name[], char path[]) {
    CommandNodePtr newPtr;
    CommandNodePtr previousPtr;
    CommandNodePtr currentPtr;

    newPtr = malloc(sizeof(CommandNode));

    if (newPtr != NULL) {
        strcpy(newPtr->name, name);
        strcpy(newPtr->path, path);
        newPtr->nextCommand = NULL;

        previousPtr = NULL;
        currentPtr = *sPtr;

        /* loop to find the correct location to insert the command alphabetically */
        while (currentPtr != NULL && (strcmp(name, currentPtr->name)) > 0) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextCommand;
        }

        /* insert the command at the beginning if it's initially empty */
        if (previousPtr == NULL) {
            newPtr->nextCommand = *sPtr;
            *sPtr = newPtr;
        }
        /* insert the new node between previousPtr and currentPtr */
        else {
            previousPtr->nextCommand = newPtr;
            newPtr->nextCommand = currentPtr;
        }
    }

    else {
        printf("The command %s not inserted. No memory available!\n", name);
    }
}

CommandNodePtr findCommand(CommandNodePtr currentCommand, char *name) {
    /* if list is empty */
    if (currentCommand == NULL) {
        printf("Command list is empty");
    }

    else {
        while ( currentCommand != NULL ) {
            if (strcmp(currentCommand->name, name) == 0)
                return currentCommand;

            currentCommand = currentCommand->nextCommand;
        }

    }

    return NULL;
}

void printCommands(CommandNodePtr currentCommand) {
    /* if list is empty */
    if (currentCommand == NULL) {
        printf("Command list is empty");
    }

    else {
        while ( currentCommand != NULL ) {
            printf("%s", currentCommand->path);
            printf("\n");
            currentCommand = currentCommand->nextCommand;
        }

    }
}
