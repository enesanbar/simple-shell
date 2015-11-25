#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include "paths.h"

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* Path list */
void setup(char inputBuffer[], char *args[], int *background);
void setupPath();
bool DoesCommandExist(char path[128]);

int main(void)
{
    char inputBuffer[MAX_LINE]; /* buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    pid_t child_pid;

    /* store the value of PATH environment variable in queue */
    setupPath();

    while (1){
        background = 0;
        printf("$ ");
        fflush(stdout);

        /* setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        /** fork a child process using fork() */
        child_pid = fork();
        if (child_pid == -1) {
            perror("Failed to fork");
            return EXIT_FAILURE;
        }

        /* child process will invoke execv() */
        if (child_pid == 0) {
            printf("Hey! I'm the child process: %ld\n", (long)getpid());
            while (headPaths != NULL) {
                char currentPath[128];
                strcpy(currentPath, headPaths->path);
                strcat(currentPath, "/");
                strcat(currentPath, args[0]);

                /* execute the given command if it exists in the path */
                if (DoesCommandExist(currentPath)) {
                    printf("executing execv\n");
                    execv(currentPath, &args[0]);
                }

                printf("not found: %s\n", currentPath);

                headPaths = headPaths->nextPath;
            }

            perror("Command not found!");
        }

        if (child_pid > 0) {
            printf("Hey! I'm the parent waiting for my child to finish! %ld\n", (long) getpid());
            wait(NULL);
            printf("My child (%ld) finished executing!\n", (long)child_pid);
        }

    }
}

/* check if the file exists in the given path */
bool DoesCommandExist(char path[128]) {
    if ( access(path, F_OK) != -1 ) {
        printf("Command %s exists\n", path);
        return true;
    }

    return false;
}

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[], int *background)
{
    int length; /* # of characters in the command line */
    int i;      /* loop index for accessing inputBuffer array */
    int start;  /* index where beginning of next command parameter is */
    int ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

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
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
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
    args[ct] = NULL; /* just in case the input line was > 80 */

    for (i = 0; i <= ct; i++)
        printf("args %d = %s\n", i, args[i]);
}

void setupPath() {
    // Get the PATH environment variable.
    char *paths = getenv("PATH");

    /* split the path names by colon (:) */
    char *token = strtok(paths, ":");
    insertIntoPath(&headPaths, &tailPaths, token);

    while (token != NULL) {
        token = strtok(NULL, ":");
        insertIntoPath(&headPaths, &tailPaths, token);
    }

}
