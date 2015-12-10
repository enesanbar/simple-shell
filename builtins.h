struct builtin_node {
    char *name;
    int *(*function) (char *args[]);
    struct builtin_node *nextBuiltIn;
};

typedef struct builtin_node BuiltInNode;
typedef BuiltInNode *BuiltinNodePtr;

BuiltinNodePtr builtin_commands = NULL;

/* insert a new built-in command to the built-in command linked-list */
void insertIntoBuiltins(BuiltinNodePtr *headPtr, char *name, int *(*function) (char *args[])) {
    /* pointer to a new node */
    BuiltinNodePtr newPtr;

    /* allocate a memory location for a new node */
    newPtr = malloc( sizeof(BuiltInNode) );

    /* if the memory is available */
    if ( newPtr != NULL ) {
        /* allocate a memory location for the command name */
        newPtr->name = malloc(strlen(name) + 1);

        /* copy the command name to the allocated memory area */
        strcpy(newPtr->name, name);

        /* assign the address of the function of the command */
        newPtr->function = function;

        /* insert the commands on top of each other (STACK) */
        newPtr->nextBuiltIn = *headPtr;
        *headPtr = newPtr;
    }

    /* if there's not enough memory to create a new node */
    else {
        fprintf(stderr, "The command %s not inserted. No memory available!\n", name);
    }
}

/* find the built-in command by its name */
BuiltinNodePtr findBuiltIn(char *name) {
    /* get a reference to the head of the built-in command linked-list */
    BuiltinNodePtr builtin = builtin_commands;

    /* loop through the built-in command linked-list */
    while (builtin != NULL) {

        /* when found, return it */
        if (strcmp(builtin->name, name) == 0)
            return builtin;

        /* proceed to the next built-in node*/
        builtin = builtin->nextBuiltIn;
    }

    /* if not found, return NULL */
    return NULL;
}