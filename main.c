#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <dirent.h>
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

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    pid_t child_pid;
    int number_of_args;

    /* store the value of PATH environment variable in queue */
    setupPath();

    /* setup all commands available */
    setupSystemFiles();
    printSystemFiles(files);

    while (1){
        printf("$ ");
        fflush(stdout);

        /* setup() calls exit() when Control-D is entered */
        number_of_args = setup(inputBuffer, args, &background);

        /* empty the command list */
        emptyCommands(&headCommand, &tailCommand);

        /* split the commands by the pipe and redirection symbols */
        /* and store them in the queue */
        parseCommands(args, number_of_args);



        FileNodePtr file = findFile(files, args[0]);

        if (file == NULL) {
            printf("%s: command not found!\n", args[0]);
            continue;
        }

        /** fork a child process using fork() */
        child_pid = fork();

        if (child_pid == -1) {
            perror("Failed to fork");
            return EXIT_FAILURE;
        }

        /* child process will invoke execv() */
        if (child_pid == 0) {
            printf("Hey! I'm the child process: %ld\n", (long)getpid());

            /* if the process is a background process, store it. */
            if (background) insertIntoProcesses(&headProcesses, &tailProcesses, getpid(), args[0]);

            /* execute the given command */
            execv(file->path, &args[0]);

            perror("Command not found!");
        }

        if (child_pid > 0) {
            if (background) {
                printf("Hey! I'm the parent (%ld) waiting for my child to finish!\n", (long) getpid());

                __pid_t pid = wait(NULL);
                fprintf(stderr, "The PID that wait(NULL) returns: %ld\n", (long)pid);

                printf("Hey! I'm the parent (%ld). My child (%ld) just finished executing!\n", (long)getpid(), (long)pid);
            } else {
                printf("Hey! I'm parent. I'm not waiting for my child.\n");
            }
        }


    }
}

/* determine if a given argument is either redirection or pipe symbol */
int isSymbol(char *symbol) {
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

/* split all command by the pipe and redirection symbols */
void parseCommands(char *args[], int number_of_args) {
    int count;
    int i;

    count = 0;
    char *temp[MAX_LINE/2 + 1];

    /* loop through the argument array */
    for (i = 0; i < number_of_args; i++) {
        /* split commands by the redirection or pipe symbol */
        /* ignore the remaining commands after ampersand */
        if (!isSymbol(args[i]) && strcmp(args[i], "&") != 0) {
            temp[count] = malloc(sizeof(args[i]));
            strcpy(temp[count], args[i]);
            count++;

            /* if it is the last argument, assign NULL to the symbol in the data structure */
            if (i + 1 == number_of_args) {
                insertIntoCommands(&headCommand, &tailCommand, temp, NULL, count);
                count = 0;  // reset the counter
            }

        }
        else {
            insertIntoCommands(&headCommand, &tailCommand, temp, args[i], count);
            memset(temp, 0, sizeof temp);
            count = 0;  // reset the counter
        }
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

        /* copy current path to the currentPath variable */
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

    for (i = 0; i <= count; i++)
        printf("args %d = %s\n", i, args[i]);

    return count;
}