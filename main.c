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
#define CREATE_FLAGS_TRUNC (O_WRONLY | O_CREAT | O_APPEND)

int setup(char inputBuffer[], char *args[], int *background);

/* Setup the path and command lists */
void setupPath();
void setupSystemFiles();

/* determine if a given argument is either redirection or pipe symbol */
int isSymbol(char *symbol);

void parseCommands(char *args[], int i);

void initializeFiles();

int executePipes();

int getNumberOfPipes(CommandNodePtr headCommand);

bool isRedirect(char *symbol);

int redirectOutput(char *redirect_type, char *filename);

int executeSingleCommand(int background, char *args[]);

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

        /* setup() calls exit() when Control-D is entered */
        number_of_args = setup(inputBuffer, args, &background);

        /* split the commands by the pipe and redirection symbols */
        /* and store them in the queue */
        parseCommands(args, number_of_args);

        /* if a single command without pipe or redirection is given */
        if (tailCommand->index == 0) {
            executeSingleCommand(background, args);
        } else {
            executePipes();
        }

    }
}

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
    if (child_pid > 0 || !background) {
        wait(NULL);
    }

    // TODO: Remove completed background process from process list

    return EXIT_SUCCESS;
}

int executePipes() {
    int i, j;
    int status;

    pid_t child_pid;
    int number_of_pipes = getNumberOfPipes(headCommand);

    int writeCounter = 1;
    int readCounter = 0;

    /* setup the pipes */
    int pipes[2 * number_of_pipes];
    for (i = 0; i < number_of_pipes; i++) {
        if (pipe(pipes + i*2) == -1) {
            perror("Failed to create the pipe");
            return EXIT_FAILURE;
        }
    }

    /* having executed this command: ls | sort | wc */
    /* we'll have 4 fds */
    // pipes[0] = read end of ls->sort pipe (read by sort)
    // pipes[1] = write end of ls->sort pipe (written by ls)
    // pipes[2] = read end of sort->wc pipe (read by wc)
    // pipes[3] = write end of sort->wc pipe (written by sort)

    CommandNodePtr command;
    for (j = 0; j <= number_of_pipes; j++) {

        if ((child_pid = fork()) == -1) {
            perror("Failed to fork!");
            return EXIT_FAILURE;
        }

        if (child_pid == 0) {
            fprintf(stderr, "I'm the child (%ld)\n", (long)getpid());

            command = headCommand;

            // command1 | command2 | command3 | command4 | command5 | command 6
            //        1   0      3   2      5   4      7   6      9    8
            if (j == 0) {
                // replace current stdout with write part of 0th pipe
                if ( (dup2(pipes[writeCounter], STDOUT_FILENO)) < 0 )
                    perror("dup2 error!\n");

            } else if (j == number_of_pipes) {
                // replace current stdin with input read of last pipe
                if ( (dup2(pipes[readCounter], STDIN_FILENO)) < 0 )
                    perror("dup2 error\n");

                // if there's redirection at the end
                if (isRedirect(headCommand->next_symbol)) {
                    pipes[writeCounter] =
                            redirectOutput(headCommand->next_symbol, headCommand->nextCommand->args[0]);

                    if (strcmp(headCommand->next_symbol, ">&") != 0) {
                        if (dup2(pipes[writeCounter], STDOUT_FILENO) == -1)
                            perror("dup2 error\n");
                    } else {
                        if (dup2(pipes[writeCounter], STDERR_FILENO) == -1)
                            perror("dup2 error\n");
                    }

                }

            } else {
                // replace current stdin with input read of middle pipe
                if ( (dup2(pipes[readCounter], STDIN_FILENO)) < 0)
                    perror("dup2 error!\n");

                // replace current stdout with write end of middle pipe
                if ( (dup2(pipes[writeCounter], STDOUT_FILENO)) < 0 )
                    perror("dup2 error\n");

            }

            /* close all pipes */
            for (i = 0; i < (2 * number_of_pipes); i++) {
                close(pipes[i]);
            }

            FileNodePtr file = findFile(files, command->args[0]);

            /* check if the given command exists */
            if (file == NULL) {
                printf("%s: command not found!\n", command->args[0]);
                headCommand = headCommand->nextCommand;
                continue;
            }

            /* execute the command */
            execv(file->path, &command->args[0]);
            fprintf(stderr, "Failed to execute the command %s\n", file->name);

            return EXIT_FAILURE;
        }
        /* parent code */
        else {
            /* update the counters in the parent code. */
            if (j == 0)
                writeCounter += 2;
            else if (j == number_of_pipes);
            else {
                readCounter += 2;
                writeCounter += 2;
            }

            headCommand = headCommand->nextCommand;
        }

    }

    /* close all pipes */
    for (i = 0; i < (2 * number_of_pipes); i++)
        close(pipes[i]);

    /* the parent will wait for all of its children*/
    for (i = 0; i <= number_of_pipes; i++){
        fprintf(stderr, "I'm the parent (%ld) and waiting for my child to finish!\n", (long)getpid());
        __pid_t child = wait(&status);
        fprintf(stderr, "I'm the parent (%ld) and my child (%d) finished executing\n", (long)getpid(), child);
    }

    return EXIT_SUCCESS;
}

int redirectOutput(char *redirect_type, char *filename) {

    if (strcmp(redirect_type, ">") == 0)
        return open(filename, CREATE_FLAGS_TRUNC, CREATE_MODE);
    else if (strcmp(redirect_type, ">>") == 0 || strcmp(redirect_type, ">&") == 0)
        return open(filename, CREATE_FLAGS_APPEND, CREATE_MODE);

    return NULL;
}


int getNumberOfPipes(CommandNodePtr headCommand) {
    int count = 0;

    /* loop through the command list*/
    while (headCommand != NULL) {
        /* if it is not the last argument and the symbol is a pipe, increment the counter */
        if (headCommand->next_symbol != NULL && strcmp(headCommand->next_symbol, "|") == 0)
            count++;

        headCommand = headCommand->nextCommand;
    }

    return count;
}

void initializeFiles() {
    /* store the value of PATH environment variable in queue */
    setupPath();

    /* setup all commands available */
    setupSystemFiles();
}

/* determine if a given argument is either redirection or pipe symbol */
int isSymbol(char *symbol) {
    /* symbols available */
    char *symbols[] = {">", ">>", "<", ">&", "|"};
    int length = sizeof (symbols) / sizeof (*symbols);
    int i;

    for (i = 0; i < length; i++) {
        if (strcmp(symbols[i], symbol) == 0) {
            return true;
        }
    }

    return false;
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

/* split all command by the pipe and redirection symbols */
void parseCommands(char *args[], int number_of_args) {
    int count;
    int i, j;

    count = 0;
    char **temp = NULL;
    temp = malloc(sizeof(char*) * number_of_args);

    /* loop through the argument array */
    for (i = 0; i < number_of_args; i++) {
        /* ignore the remaining commands after ampersand */
        if (strcmp(args[i], "&") == 0) return;

        /* split commands by the redirection or pipe symbol */
        if (!isSymbol(args[i])) {
            temp[count] = malloc(strlen(args[i]) + 1);
            strcpy(temp[count], args[i]);
            count++;

            /* if it is the last argument, assign NULL to the symbol in the data structure */
            if (i + 1 == number_of_args) {
                insertIntoCommands(&headCommand, &tailCommand, temp, NULL, count);
                for (j = 0; j < count; j++) free(temp[j]);
                count = 0;  // reset the counter
            }

        }
        else {
            insertIntoCommands(&headCommand, &tailCommand, temp, args[i], count);
            for (j = 0; j < count; j++) free(temp[j]);
            count = 0;  // reset the counter
        }
    }

    free(temp);
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
                if (inputBuffer[i] == '&'){
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    args[count] = NULL; /* just in case the input line was > 80 */

    /* empty the command list for the next use */
    emptyCommands(&headCommand, &tailCommand);

    for (i = 0; i <= count; i++)
        printf("args %d = %s\n", i, args[i]);

    return count;
}