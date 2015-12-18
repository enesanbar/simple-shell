#include <stdbool.h>

#define PIPE "|"

/* Struct to construct a list of commands. */
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

int isCommandEmpty(CommandNodePtr headPtr);

bool isRedirect(char *symbol);

void insertIntoCommands( CommandNodePtr *headPtr, CommandNodePtr *tailPtr, char **args, int count );

void clearCommands(CommandNodePtr *headPtr, CommandNodePtr *tailPtr);