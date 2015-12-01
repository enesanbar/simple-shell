#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
#include <fcntl.h>
#include "paths.h"
#include "processes.h"
#include "files.h"
#include "command.h"

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
// Open the file to write, if it does not exists create, if it does append to it.
#define CREATE_FLAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)

// Open the file to write, if it does not exists create, if it does truncate it.
#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)

int setup(char inputBuffer[], char *args[], int *background);

/* Setup the path and command lists */
void initializeFiles();

void parseCommands(char *args[], int i);

int executeCommands(int background);

int redirectOutput(char *redirect_type, char *filename);

int executeSingleCommand(int background, char *args[]);

void remove_ampersand(char *args[], int background, int length);

int isBackgroundProcess(char *args[], int number_of_arguments);

bool isRedirect(char *symbol);

bool isPipe(char *symbol);

bool isFile(char *input);

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int number_of_args;

    /* store the files in the path in the linked-list */
    initializeFiles();

    while (1){
        printf("$ ");
        fflush(stdout);

        /* get the number of arguments */
        number_of_args = setup(inputBuffer, args, &background);

        /* empty the command queue for the next use */
        emptyCommands(&commandHead, &commandTail);

        /* set the location of ampersand and remove it from the arguments array */
        if ( (background = isBackgroundProcess(args, number_of_args)) > 0 ) {
            remove_ampersand(args, background, number_of_args);
        }

        /* split the arguments by the pipe symbol */
        /* and store them in the queue */
        parseCommands(args, number_of_args);

        /* if no argument is provided, don't execute and show a new command prompt */
        if (number_of_args == 0) continue;

        /* start executing of the commands */
        executeCommands(background);
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

    /* the current child PID */
    pid_t child_pid;

    /* the PID of the child that's finished execution */
    pid_t finished_child;

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

        /* fork a new child, exit on failure. */
        if ((child_pid = fork()) == -1) {
            perror("Failed to fork!");
            exit(EXIT_FAILURE);
        }

        /* child code */
        if (child_pid == 0) {
            // command1 | command2 | command3 | command4 | command5 | command 6
            //        1   0      3   2      5   4      7   6      9    8

            /* if the process is running in the background, store it. */
            if (background) insertIntoProcesses(&headProcesses, &tailProcesses, getpid(), commands->args[0]);


            /* setup the input and output end of the pipes for both pipe and redirection operations */

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

                if (pipes[pipeUsed] != NULL) {
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

            /* find the system file such as ls and sort */
            FileNodePtr file = findFile(files, commands->args[0]);

            /* check if the given command exists */
            if (file == NULL) {
                printf("%s: command not found!\n", commands->args[0]);
                commands = commands->nextCommand;
                continue;
            }

            /* execute the command: file node includes path as well as the filename */
            execv(file->path, &commands->args[0]);
            fprintf(stderr, "Failed to execute the command %s\n", file->name);

            return EXIT_FAILURE;
        }

        /* advance to the next command */
        commands = commands->nextCommand;
    }

    /* the parent closes all pipes */
    for (i = 0; i < (2 * number_of_pipes); i++)
        close(pipes[i]);


    // TODO: Handle background processes here later

    /* the parent will wait for all of its children */
    while (wait(NULL) > 0);

    return EXIT_SUCCESS;
}

/* unnecessary function */
int executeSingleCommand(int background, char *args[]) {
    pid_t child_pid;

    /* fork a child process */
    if ((child_pid = fork()) == -1) {
        perror("Failed to fork");
        return EXIT_FAILURE;
    }

    /* child process will invoke execv() */
    if (child_pid == 0) {
        /* if the process is running in the background, store it. */
        if (background) insertIntoProcesses(&headProcesses, &tailProcesses, getpid(), args[0]);

        FileNodePtr file = findFile(files, args[0]);

        /* execute the given command */
        execv(file->path, &args[0]);

        perror("Command not found!");
    }

    /* parent will wait if ampersand does not exist in the command*/
    if (child_pid > 0 && !background) {
        wait(NULL);
    }

    // TODO: Remove completed background process from process list

    return EXIT_SUCCESS;
}

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

    return NULL;
}

/* return the index of ampersand symbol */
int isBackgroundProcess(char *args[], int number_of_arguments) {
    int i;
    for (i = 0; i < number_of_arguments; i++) {
        if (strcmp(args[i], "&") == 0) {
            return i;
        }
    }

    return 0;
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

void initializeFiles() {
    /* store the value of PATH environment variable in queue */
    setupPath();

    /* setup all commands available */
    setupSystemFiles();
}

/* read the next command line; separate it into distinct arguments. */
int setup(char inputBuffer[], char *args[], int *background)
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