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
#include "command.h"

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */

/* Path list */
int setup(char inputBuffer[], char *args[], int *background);
void setupPath();

int isBackgroundProcess(char *args[], int i);
void remove_ampersand(char *args[], int background, int length);

void setupCommands();

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
    setupCommands();

    while (1){
        printf("$ ");
        fflush(stdout);

        /* setup() calls exit() when Control-D is entered */
        number_of_args = setup(inputBuffer, args, &background);

        /* set if the process will run in background */
        background = isBackgroundProcess(args, number_of_args);

        if (background) {
            remove_ampersand(args, background, number_of_args);
        }

        CommandNodePtr command = findCommand(commands, args[0]);

        if (command == NULL) {
            printf("%s: command not found!\n", args[0]);
        } else {
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
                execv(command->path, &args[0]);

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
}

/**/
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

/* read the path to store all commands in the system */
void setupCommands() {
    while (headPaths->path != NULL) {

        char currentPath[128];
        strcpy(currentPath, headPaths->path);
        strcat(currentPath, "/");

        DIR *dir;
        struct dirent *ent;
        if ((dir = opendir (currentPath)) != NULL) {
            /* print all the files within directory */
            while ((ent = readdir (dir)) != NULL) {
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;

                char path[512];
                strcpy(path, currentPath);
                strcat(path, ent->d_name);
                insertCommand(&commands, ent->d_name, path);
            }
            closedir (dir);
        }

        /* advance to the next path in the path list */
        headPaths = headPaths->nextPath;
    }
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

/* read the next command line; separate it into distinct arguments. */
int setup(char inputBuffer[], char *args[], int *background)
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

    return ct;
}