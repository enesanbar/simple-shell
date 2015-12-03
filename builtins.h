struct builtin_node {
    char *name;
    int *(*function) (char *args[]);
    struct builtin_node *nextBuiltIn;
};

typedef struct builtin_node BuiltInNode;
typedef BuiltInNode *BuiltinNodePtr;

BuiltinNodePtr builtin_commands = NULL;

void insertIntoBuiltins(BuiltinNodePtr *sPtr, char *name, int *(*function) (char *args[])) {
    BuiltinNodePtr newPtr;
    BuiltinNodePtr previousPtr;
    BuiltinNodePtr currentPtr;

    newPtr = malloc( sizeof(BuiltInNode) );

    if ( newPtr != NULL ) {
        newPtr->name = malloc(strlen(name) + 1);
        strcpy(newPtr->name, name);

        newPtr->function = function;
        newPtr->nextBuiltIn = NULL;

        previousPtr = NULL;
        currentPtr = *sPtr;

        /* loop to find the correct location to insert the command alphabetically */
        while ( currentPtr != NULL && (strcmp(name, currentPtr->name)) > 0) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextBuiltIn;
        }

        /* insert the command at the beginning if it's initially empty */
        if (previousPtr == NULL) {
            newPtr->nextBuiltIn = *sPtr;
            *sPtr = newPtr;
        }

        else {
            previousPtr->nextBuiltIn = newPtr;
            newPtr->nextBuiltIn = currentPtr;
        }
    } else {
        fprintf(stderr, "The command %s not inserted. No memory available!\n", name);
    }
}

BuiltinNodePtr findBuiltIn(char *name) {
    BuiltinNodePtr builtin = builtin_commands;

    while (builtin != NULL) {

        if (strcmp(builtin->name, name) == 0) {
            return builtin;
        }

        builtin = builtin->nextBuiltIn;
    }

    return NULL;
}

void printBuiltins(BuiltinNodePtr builtins) {
    /* if there's no built-in function */
    if (builtins == NULL) {
        printf("Command list is empty");
    }

    else {
        fprintf(stderr, "Here's the built-in commands supported:\n");

        while ( builtins != NULL ) {
            //builtins->function("");
            fprintf(stderr, "%s\n", builtins->name);

            builtins = builtins->nextBuiltIn;
        }

    }
}
