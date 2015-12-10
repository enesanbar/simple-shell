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

CommandNodePtr commandHead = NULL; /* initialize the head of the command queue */
CommandNodePtr commandTail = NULL; /* initialize the tail of the command queue */

int isCommandEmpty(CommandNodePtr headPtr) {
    return headPtr == NULL;
}

/* determine if the given string is a redirection symbol */
bool isRedirect(char *symbol) {
    /* symbols available */
    char *symbols[] = {">", ">>", "<", ">&"};

    /* lenght of the symbols array (just to be more dynamic) */
    int length = sizeof (symbols) / sizeof (*symbols);

    /* loop counter */
    int i;

    /* loop through the symbols array */
    for (i = 0; i < length; i++)
        if (strcmp(symbols[i], symbol) == 0)
            return true;

    /* if not found, return false (0) */
    return false;
}


void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char **args, int count ) {
    int i, number_of_args = 0;

    /* pointer to a new command */
    CommandNodePtr newPtr = NULL;

    /* allocate memory for the new command */
    newPtr = malloc( sizeof(CommandNode) );

    /* no redirection assumed at first */
    newPtr->input = NULL;
    newPtr->output_type = NULL;
    newPtr->output = NULL;

    if (newPtr != NULL) {
        /* allocate memory for each argument */
        newPtr->args = malloc(sizeof(char*) * (count + 1));

        /* parse the command by the redirect symbols and
         * store them appropriate place in the command list */

        /* if the command is as follows:
         * wc -l < input.txt > output.txt
         * this loop slices the 1st part (wc -l) and store it in the args field */
        for (i = 0; i < count; i++) {
            if (isRedirect(args[i])) break;

            newPtr->args[i] = malloc(strlen(args[i]) + 1);
            strcpy(newPtr->args[i], args[i]);
            number_of_args++;
        }

        newPtr->args[i] = NULL; /* argument array must be terminated with NULL */

        /* determine the input file and the output file if there's any.
         * this loop slices
         * the 2nd part (input.txt) and store it in the input field
         * the 3rd part (output.txt) and store it in the output field */
        for (; i < count; i++) {
            /* check if there's an input redirection symbol */
            if (strcmp(args[i], "<") == 0) {

                /* check if there's not a filename after the symbol or
                 * there's another redirection symbol (> >) written mistakenly */
                if (args[i + 1] == NULL || isRedirect(args[i + 1])) {
                    fprintf(stderr, "parse error\n");
                    exit(EXIT_FAILURE);
                }

                /* store the name of the input file */
                /* allocate memory for the name of the file */
                newPtr->input = malloc(strlen(args[i + 1]) + 1);

                /* copy the filename to the designated area `*/
                strcpy(newPtr->input, args[i + 1]);
            }

            /* check if there's an output redirection symbol */
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], ">&") == 0) {
                /* check if there's not a filename after the symbol or
                 * there's another redirection symbol (> >) written mistakenly */
                if (args[i + 1] == NULL || isRedirect(args[i + 1])) {
                    fprintf(stderr, "parse error\n");
                    exit(EXIT_FAILURE);
                }

                /* allocate memory for the redirection symbol */
                newPtr->output_type = malloc(strlen(args[i]) + 1);
                /* copy the redirection symbol to the allocated area */
                strcpy(newPtr->output_type, args[i]);

                /* allocate memory for the filename */
                newPtr->output = malloc(strlen(args[i + 1]) + 1);
                /* copy the file name to the allocated area*/
                strcpy(newPtr->output, args[i + 1]);
            }
        }

        /* if the command queue is empty, insert the command at head */
        if ( isCommandEmpty(*headPtr) ) {
            newPtr->index = 0; /* give 0-based index to the commands */
            *headPtr = newPtr;
        } else {
            newPtr->index = (*tailPtr)->index + 1;
            (*tailPtr)->nextCommand = newPtr;
        }

        newPtr->number_of_args = number_of_args;
        newPtr->nextCommand = NULL;
        *tailPtr = newPtr;
    }

    /* if there's not enough memory to create a new node */
    else {
        printf("command not inserted. No memory available.\n");
    }
}

/* this function is used to clear out the command queue for the next use. */
void clearCommands(CommandNodePtr *headPtr, CommandNodePtr *tailPtr) {
    /* get a reference to the head of the command queue */
    CommandNodePtr temp;

    /* loop counter */
    int i;

    /* loop through the command queue */
    while (*headPtr != NULL) {
        /* store the address of the current node in a temporary pointer */
        temp = *headPtr;

        /* assign the next node as the head of the queue */
        *headPtr = (*headPtr)->nextCommand;

        /* if queue is empty, both head and tail will be NULL */
        if (*headPtr == NULL)
            *tailPtr = NULL;

        /* free up the memory:
         * "ideally free() everything that's allocated with malloc()" */
        for (i = 0; i < temp->number_of_args; ++i)
            free(temp->args[i]);

        /* free the holder array */
        free(temp->args);

        /* these fields are also allocated dynamically with malloc() */
        free(temp->input);
        free(temp->output);
        free(temp->output_type);

        /* finally, free the node */
        free(temp);
    }
}