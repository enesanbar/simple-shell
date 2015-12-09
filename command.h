#define PIPE "|"

/* Struct to construct a list of paths. */
struct command_node {
    int index;
    char **args;
    int number_of_args;
    char *input;
    char *output;
    char *output_type;
    struct command_node *nextCommand;
};

typedef struct command_node CommandNode; /* synonym for struct command_node */
typedef CommandNode *CommandNodePtr; /* synonym for CommandNode* */

CommandNodePtr commandHead = NULL; /* initialize headPtr */
CommandNodePtr commandTail = NULL; /* initialize tailPtr */

int isCommandEmpty(CommandNodePtr headPtr) {
    return headPtr == NULL;
}

bool isRedirect(char *symbol) {
    /* symbols available */
    char *symbols[] = {">", ">>", "<", ">&"};
    int length = sizeof (symbols) / sizeof (*symbols);
    int i;

    for (i = 0; i < length; i++) {
        if (strcmp(symbols[i], symbol) == 0) {
            return true;
        }
    }

    return false;
}


void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char **args, int count ) {
    int i, number_of_args = 0;
    CommandNodePtr newPtr = NULL;   /* pointer to a new command */

    /* allocate memory for the new node */
    newPtr = malloc( sizeof(CommandNode) );

    newPtr->input = NULL;
    newPtr->output_type = NULL;
    newPtr->output = NULL;

    if (newPtr != NULL) {
        /* allocate memory for each argument */
        newPtr->args = malloc(sizeof(char*) * (count + 1));

        /* parse the command by the redirect symbols and
         * store them appropriate place in the struct */

        /* if the command like: wc -l < input.txt > output.txt */
        /* this loop slices the first part (wc -l) and store it in the args */
        for (i = 0; i < count; i++) {
            if (isRedirect(args[i])) break;

            newPtr->args[i] = malloc(strlen(args[i]) + 1);
            strcpy(newPtr->args[i], args[i]);
            number_of_args++;
        }
        newPtr->args[i] = NULL; /* argument array must be terminated with NULL */

        /* determine the input file and the output file if there's any */
        for (; i < count; i++) {
            if (strcmp(args[i], "<") == 0) {
                if (args[i + 1] == NULL || isRedirect(args[i + 1])) {
                    fprintf(stderr, "parse error\n");
                    exit(EXIT_FAILURE);
                }

                newPtr->input = malloc(strlen(args[i + 1]) + 1);
                strcpy(newPtr->input, args[i + 1]);
            }

            if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], ">&") == 0) {
                if (args[i + 1] == NULL || isRedirect(args[i + 1])) {
                    fprintf(stderr, "parse error\n");
                    exit(EXIT_FAILURE);
                }

                newPtr->output_type = malloc(strlen(args[i]) + 1);
                strcpy(newPtr->output_type, args[i]);

                newPtr->output = malloc(strlen(args[i + 1]) + 1);
                strcpy(newPtr->output, args[i + 1]);
            }
        }

        /* if empty, insert node at head */
        if (isCommandEmpty(*headPtr)) {
            newPtr->index = 0;
            *headPtr = newPtr;
        } else {
            newPtr->index = (*tailPtr)->index + 1;
            (*tailPtr)->nextCommand = newPtr;
        }

        newPtr->number_of_args = number_of_args;
        newPtr->nextCommand = NULL;
        *tailPtr = newPtr;
    } else {
        printf( "command not inserted. No memory available.\n");
    }
}

void emptyCommands(CommandNodePtr *headPtr, CommandNodePtr *tailPtr) {
    /* create a temporary variable not to change the original pointer */
    CommandNodePtr temp;
    int i;

    while (*headPtr != NULL) {
        temp = *headPtr;
        *headPtr = (*headPtr)->nextCommand;

        /* if queue is empty */
        if (*headPtr == NULL) {
            *tailPtr = NULL;
        }

        /* free up the memory: ideally free() everything that's allocated with malloc() */
        for (i = 0; i < temp->number_of_args; ++i) {
            free(temp->args[i]);
        }

        free(temp->input);
        free(temp->output);
        free(temp->output_type);
        free(temp->args);   /* free the holder array */
        free(temp);         /* free the node */
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
            fprintf(stderr, "index: %d\n", currentCommand->index);

            for (i = 0; i <= currentCommand->number_of_args; i++)
                fprintf(stderr, "args %d = %s\n", i, currentCommand->args[i]);

            if (currentCommand->input != NULL)
                fprintf(stderr, "currentCommand->input: %s\n", currentCommand->input);
            if (currentCommand->output_type != NULL)
                fprintf(stderr, "currentCommand->output_type: %s\n", currentCommand->output_type);
            if (currentCommand->output != NULL)
                fprintf(stderr, "currentCommand->output: %s\n", currentCommand->output);

            currentCommand = currentCommand->nextCommand;
        }

    }
}