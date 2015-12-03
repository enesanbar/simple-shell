#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include <regex.h>
#include "paths.h"
#include "files.h"
#include "command.h"
#include "processes.h"
#include "builtins.h"

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
// Open the file to write, if it does not exists create, if it does append to it.
#define CREATE_FLAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)

// Open the file to write, if it does not exists create, if it does truncate it.
#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)

int setup(char inputBuffer[], char *args[]);

/* Setup the path and command lists */
void initializeCommands();

void parseCommands(char *args[], int i);

int executeCommands(int background);

int redirectOutput(char *redirect_type, char *filename);

void remove_ampersand(char *args[], int background, int length);

int isBackgroundProcess(char *args[], int number_of_arguments);

bool isRedirect(char *symbol);

bool isPipe(char *symbol);

bool isFile(char *input);

void updateRunningProcesses();

int main(void)
{
    BuiltinNodePtr builtin;
    char inputBuffer[MAX_LINE]; /* buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int number_of_args;

    /* store the files in the path in the linked-list */
    initializeCommands();

    while (1){
        printf("$ ");
        fflush(stdout);

        /* get the number of arguments */
        number_of_args = setup(inputBuffer, args);

        updateRunningProcesses();

        /* if no argument is provided, don't execute and show a new command prompt */
        if (number_of_args == 0) continue;

        /* empty the command queue for the next use */
        emptyCommands(&commandHead, &commandTail);

        /* set the location of ampersand and remove it from the arguments array */
        if ( (background = isBackgroundProcess(args, number_of_args)) > 0 ) {
            remove_ampersand(args, background, number_of_args);
            number_of_args--;   /* not interested in the count of & symbol */
        }

        /* if the command is a built-in command, execute its function */
        if ((builtin = findBuiltIn(args[0])) != NULL) {
            builtin->function(args);
        }
        /* if it's not a built-in function, it should be either a system function or not */
        else {
            /* split the arguments by the pipe symbol */
            /* and store them in the queue */
            parseCommands(args, number_of_args);

            /* start executing of the commands */
            executeCommands(background);
        }

    }
}

/* split all command by the pipe symbol */
void parseCommands(char *args[], int number_of_args) {
    int count;
    int i, j;

    count = 0;
    char **temp = NULL;
    temp = malloc(sizeof(char*) * number_of_args);

    /* loop through the argument array */
    for (i = 0; i <= number_of_args; i++) {
        /* split commands by the pipe symbol */
        if (args[i] != NULL && ! isPipe(args[i])) {
            temp[count] = malloc(strlen(args[i]) + 1);
            strcpy(temp[count], args[i]);
            count++;
        }
        else {
            insertIntoCommands(&commandHead, &commandTail, temp, count);
            for (j = 0; j < count; j++) free(temp[j]);
            count = 0;  // reset the counter
        }
    }

    free(temp);
}

int executeCommands(int background) {
    /* counters */
    int i, j;

    /* a temporary pointer to the head of the command queue */
    CommandNodePtr commands = commandHead;

    /* find the system file such as ls and sort */
    FileNodePtr file;

    /* the current child PID */
    pid_t child_pid;

    /* command list is zero-based indexed.
     * if there's 2 commands, there's 1 pipe */
    int number_of_pipes = commandTail->index;

    /* setup the pipes */
    int pipes[2 * number_of_pipes];
    for (i = 0; i < number_of_pipes; i++) {
        if (pipe(pipes + i*2) == -1) {
            perror("Failed to create the pipe");
            return EXIT_FAILURE;
        }
    }

    /* execute the commands one by one in a new process */
    for (j = 0; j <= number_of_pipes; j++) {

        /* check if the given command exists, exit on failure */
        if ( (file = findFile(files, commands->args[0])) == NULL ) {
            printf("%s: command not found!\n", commands->args[0]);
            return EXIT_FAILURE;
        }

        /* fork a new child, exit on failure. */
        if ( (child_pid = fork()) == -1 ) {
            perror("Failed to fork!");
            return EXIT_FAILURE;
        }

        /* child code */
        if (child_pid == 0) {
            /* setup the input and output end of the pipes for both pipe and redirection operations
               command1 | command2 | command3 | command4 | command5 | command 6
                      1   0      3   2      5   4      7   6      9   8                           */

            /* if it's not the last command, replace current stdout with input read of last pipe */
            if (commands->index != number_of_pipes) {
                if ((dup2(pipes[commands->index * 2 + 1], STDOUT_FILENO)) < 0)
                    perror("dup2 error\n");
            }

            /* if it's not the first command, replace current stdin with write part of 0th pipe */
            if (commands->index != 0) {
                if ( (dup2(pipes[(commands->index - 1) * 2], STDIN_FILENO)) < 0 )
                    perror("dup2 error!\n");
            }

            // if there's input redirection in the current command
            if (commands->input != NULL) {
                /* safekeeping not to broke pipes if the input is not a file */
                if ( isFile(commands->input) ) {
                    pipes[(commands->index - 1) * 2] = open(commands->input, O_RDONLY);

                    /* change the input end of the file descriptor to the file */
                    if ( dup2(pipes[(commands->index - 1) * 2], STDIN_FILENO) == -1 ) {
                        perror("dup2 error\n");
                    }
                }
            }

            // if there's output redirection in the current command
            if (commands->output_type != NULL && isRedirect(commands->output_type)) {
                int pipeUsed = commands->index * 2 + 1;
                pipes[pipeUsed] = redirectOutput(commands->output_type, commands->output);

                if (pipes[pipeUsed] != -1) {
                    if (strcmp(commands->output_type, ">&") != 0) {
                        if (dup2(pipes[pipeUsed], STDOUT_FILENO) == -1)
                            perror("dup2 error\n");
                    } else {
                        if (dup2(pipes[pipeUsed], STDERR_FILENO) == -1)
                            perror("dup2 error\n");
                    }
                } else {
                    perror("Error opening the file!\n");
                }
            }

            /* close all pipes */
            for (i = 0; i < (2 * number_of_pipes); i++) {
                close(pipes[i]);
            }

            /* execute the command: file node includes path as well as the filename */
            execv(file->path, &commands->args[0]);
            fprintf(stderr, "Failed to execute the command %s\n", file->name);

            return EXIT_FAILURE;
        }

        /* if the process is running in the background, store it. */
        if (background) {
            insertIntoProcesses(&processHead, &processTail, child_pid, commands->args, commands->number_of_args);
        }

        /* advance to the next command */
        commands = commands->nextCommand;
    }

    /* the parent closes all pipes */
    for (i = 0; i < (2 * number_of_pipes); i++)
        close(pipes[i]);

    /* the parent will wait for its children if & is provided */
    if (commandTail->index == 0 && background == false) {
        wait(NULL);
    }

    if (commandTail->index > 0 && number_of_pipes){
        /* the parent will wait for all of its children */
        while (wait(NULL) > 0);
    }

    return EXIT_SUCCESS;
}

void updateRunningProcesses() {
    ProcessNodePtr process = processHead;
    int status;

    while (process != NULL) {
        if (process->is_running == RUNNING) {
            if (waitpid(process->process_id, &status, WNOHANG) != 0) {
                process->is_running = FINISHED;
            }
        }

        process = process->nextProcess;
    }


}

/* determine if the given symbol is a PIPE symbol */
bool isPipe(char *symbol) {
    return strcmp(symbol, PIPE) == 0;
}

/* check if the input is a file */
bool isFile(char *input) {
    return (access(input, F_OK) != -1) ? true : false;
}

int redirectOutput(char *redirect_type, char *filename) {
    if (strcmp(redirect_type, ">") == 0)
        return open(filename, CREATE_FLAGS_TRUNC, CREATE_MODE);
    else if (strcmp(redirect_type, ">>") == 0 || strcmp(redirect_type, ">&") == 0)
        return open(filename, CREATE_FLAGS_APPEND, CREATE_MODE);

    return -1;
}

/* return the index of ampersand symbol */
int isBackgroundProcess(char *args[], int number_of_arguments) {
    int i;
    for (i = 0; i < number_of_arguments; i++) {
        if (strcmp(args[i], "&") == 0) {
            return i;
        }
    }

    return false;
}

/* remove the ampersand symbol from the arguments array */
void remove_ampersand(char *args[], int background, int length) {
    int i = background;
    for ( ; i < length; i++) {
        args[i] = NULL;
    }
}


/* setup the locations in the PATH variable */
void setupPath() {
    // Get the PATH environment variable.
    char *paths = getenv("PATH");

    /* split the path names by colon (:) */
    char *token = strtok(paths, ":");

    /* insert the first path to the queue */
    insertIntoPath(&headPaths, &tailPaths, token);

    /* keep inserting the remaining paths. */
    while (token != NULL) {
        token = strtok(NULL, ":");
        insertIntoPath(&headPaths, &tailPaths, token);
    }
}

/* read the path to store all the system's commands in linked list */
void setupSystemFiles() {
    while (headPaths->path != NULL) {

        /* allocate enough memory for the current path name */
        char *currentPath = malloc(sizeof headPaths->path + 1);

        /* copy current path in the queue to the currentPath variable */
        strcpy(currentPath, headPaths->path);   // /usr/bin

        /* append slash character to the path */
        strcat(currentPath, "/");   // /usr/bin/

        // open the current directory and start reading files
        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (currentPath)) != NULL) {
            while ((ent = readdir (dir)) != NULL) {
                // ignore the current and previous directories
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

                /* allocate enough memory for the length of the file path */
                char *file_path = malloc(sizeof currentPath + sizeof ent->d_name);

                /* copy the currentPath to the file_path pointer and append file name to it*/
                strcpy(file_path, currentPath);     // /bin/
                strcat(file_path, ent->d_name);     // /bin/ls

                /* insert the command to the linked list */
                insertFiles(&files, ent->d_name, file_path);

                /* free the memory location that file_path pointer points to */
                memset(file_path, 0, sizeof file_path);
            }
            closedir (dir);
        }

        /* advance to the next path in the path list */
        headPaths = headPaths->nextPath;

        /* free the memory location that currentPath pointer points to */
        memset(currentPath, 0, sizeof currentPath);
    }
}

int *ps_all(char *args[]) {
    ProcessNodePtr process = processHead;
    int i;

    printf("Running: \n");
    while (process != NULL) {
        /* if the process is still running */
        if (process->is_running == RUNNING) {

            printf("\t[%d]\t", process->index);

            for (i = 0; i < process->number_of_args; i++)
                printf("%s ", process->args[i]);

            printf("(PID=%ld)\n", (long)process->process_id);
        }

        /* proceed to the nex process */
        process = process->nextProcess;
    }

    process = processHead;
    printf("Finished: \n");
    while (process != NULL) {

        /* if the process is finished and has never been displayed with the ps_all command */
        if (process->is_running == FINISHED) {

            /* print the process index in square bracket */
            printf("\t[%d]\t", process->index);

            /* print the arguments of the command */
            for (i = 0; i < process->number_of_args; i++)
                printf("%s ", process->args[i]);

            printf("\n");

            /* finished process should be displayed once and removed from the data structure  */
            removeFromProcesses(&processHead, &processTail, process->process_id);
        }

        /* proceed to the nex process */
        process = process->nextProcess;
    }

}

int *kill_process(char *args[]) {
    int index;
    int process_id;
    regex_t regex_compiled;
    ProcessNodePtr process;

    // check if the argument is provided
    if (args[1] == NULL) {
        printf("Usage:\n\tkill %%index\n\tkill process_id\n");
        return (int *) EXIT_FAILURE;
    }

    // compile the regular expression which looks for %number pattern
    if (regcomp(&regex_compiled, "%[0-9]+", REG_EXTENDED)) {
        printf("Could not compile regular expression.\n");
        return (int *) EXIT_FAILURE;
    }

    /* if the argument is provided with the percent (%) symbol */
    if ( ! regexec(&regex_compiled, args[1], 0, NULL, 0) ) {
        index = atoi(strtok(args[1], "%"));

        if ( (process = findProcess(processHead, index, INDEX)) == NULL ) {
            printf("A process with an index of %d doesn't exists.\nYou may want to run 'ps_all' again!\n\n", index);
            return (int *) EXIT_FAILURE;
        }

    } else {
        process_id = atoi(args[1]);

        if ( (process = findProcess(processHead, process_id, PROCESS)) == NULL ){
            printf("A process with a process id of %d doesn't exists.\nYou may want to run 'ps_all' again!\n\n", process_id);
            return (int *) EXIT_FAILURE;
        }
    }

    process->is_running = FINISHED;
    kill(process->process_id, SIGTERM);

    return EXIT_SUCCESS;
}

int *fg(char *args[]) {
    int index;
    regex_t regex_compiled;
    ProcessNodePtr process;

    // check if the argument is provided
    if (args[1] == NULL) {
        printf("Usage:\n\tfg %%index\n");
        return (int *) EXIT_FAILURE;
    }

    // compile the regular expression which looks for %number pattern
    if (regcomp(&regex_compiled, "%[0-9]+", REG_EXTENDED)) {
        printf("Could not compile regular expression.\n");
        return (int *) EXIT_FAILURE;
    }

    /* if the argument is provided with the percent (%) symbol */
    if ( ! regexec(&regex_compiled, args[1], 0, NULL, 0) ) {
        index = atoi(strtok(args[1], "%"));

        if ( (process = findProcess(processHead, index, INDEX)) == NULL ) {
            printf("A process with an index of %d doesn't exists.\nYou may want to run 'ps_all' again!\n\n", index);
            return (int *) EXIT_FAILURE;
        }
    } else {
        printf("Usage:\n\tfg %%index\n");
        return (int *) EXIT_FAILURE;
    }

    kill(process->process_id, SIGCONT);
    removeFromProcesses(&processHead, &processTail, process->process_id);

    return EXIT_SUCCESS;
}

/* built-in exit function to exit the terminal
 * if there's no background process running */
int *exit_process(char *args[]) {

    /* a temporary variable to reference the head process node*/
    ProcessNodePtr process = processHead;

    /* loop through the processes list */
    while (process != NULL) {

        /* if there's at least one background process running */
        /* return to the caller with failure */
        if (process->is_running == RUNNING) {
            printf("There're some background processes running\nPlease terminate them first!\n\n");
            return (int *) EXIT_FAILURE;
        }

        /* advance to the next process */
        process = process->nextProcess;
    }

    /* exit the program with success */
    exit(EXIT_SUCCESS);
}

/* insert the built-in functions to the linked-list */
void setupBuiltIns() {
    insertIntoBuiltins(&builtin_commands, "ps_all", &ps_all);
    insertIntoBuiltins(&builtin_commands, "kill", &kill_process);
    insertIntoBuiltins(&builtin_commands, "fg", &fg);
    insertIntoBuiltins(&builtin_commands, "exit", &exit_process);
}

/* initialize files in the PATH */
void initializeCommands() {
    /* store the value of PATH environment variable in queue */
    setupPath();

    /* setup all commands available */
    setupSystemFiles();

    /* setup built-in commands */
    setupBuiltIns();
}

/* read the next command line; separate it into distinct arguments. */
int setup(char inputBuffer[], char *args[])
{
    int length; /* # of characters in the command line */
    int i;      /* loop index for accessing inputBuffer array */
    int start;  /* index where beginning of next command parameter is */
    int count;     /* index of where to place the next parameter into args[] */

    count = 0;

    /* read what the user enters on the command line */
    length = (int) read(STDIN_FILENO, inputBuffer, MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

    /* the signal interrupted the read system call */
    /* if the process is in the read() system call, read returns -1
    However, if this occurs, errno is set to EINTR. We can check this value
    and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    for (i = 0; i < length; i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[count] = &inputBuffer[start];    /* set up pointer */
                    count++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[count] = &inputBuffer[start];
                    count++;
                }
                inputBuffer[i] = '\0';
                args[count] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
        } /* end of switch */
    }    /* end of for */
    args[count] = NULL; /* just in case the input line was > 80 */

//    for (i = 0; i <= count; i++)
//        printf("args %d = %s\n", i, args[i]);

    return count;
}