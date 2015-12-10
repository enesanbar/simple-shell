struct file_node {
    char *name;
    char *path;
    struct file_node *nextFile;
};

typedef struct file_node FileNode; /* synonym for struct process_node */
typedef FileNode *FileNodePtr; /* synonym for FileNode* */

FileNodePtr files = NULL; /* initialize the file linked-list */

/* insert a new file to the files linked-list */
void insertFiles (FileNodePtr *sPtr, char name[], char path[]) {
    FileNodePtr newPtr;         /* pointer to the new node */
    FileNodePtr previousPtr;    /* pointer to the previous node */
    FileNodePtr currentPtr;     /* pointer to the current node*/

    /* allocate memory for the new node */
    newPtr = malloc(sizeof(FileNode));

    /* if there's enough memory for the new node*/
    if (newPtr != NULL) {
        /* allocate enough memory for the name of the file */
        newPtr->name = malloc(strlen(name) + 1);

        /* copy the name to the allocated area */
        strcpy(newPtr->name, name);

        /* allocate enough memory for the name of the file */
        newPtr->path = malloc(strlen(path) + 1);

        /* copy the path to the allocated area */
        strcpy(newPtr->path, path);

        newPtr->nextFile = NULL;

        previousPtr = NULL;
        currentPtr = *sPtr;

        /* loop to find the correct location to insert the command alphabetically */
        while (currentPtr != NULL && (strcmp(name, currentPtr->name)) > 0) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextFile;
        }

        /* insert the command at the beginning if it's initially empty */
        if (previousPtr == NULL) {
            newPtr->nextFile = *sPtr;
            *sPtr = newPtr;
        }
        /* insert the new node between previousPtr and currentPtr */
        else {
            previousPtr->nextFile = newPtr;
            newPtr->nextFile = currentPtr;
        }
    }

    /* if there's not enough memory */
    else {
        printf("The command %s not inserted. No memory available!\n", name);
    }
}

/* find the path of the system file by its name */
FileNodePtr findFile(FileNodePtr currentFile, char *name) {
    /* if list is empty */
    if (currentFile == NULL) {
        printf("Command list is empty");
    }

    else {
        /* loop through the files linked-list to find the file */
        while ( currentFile != NULL ) {

            /* the user can execute commands by either the name or full path */
            /* ls or /bin/ls both works */
            if ((strcmp(currentFile->name, name) == 0) || strcmp(currentFile->path, name) == 0)
                return currentFile;

            /* proceed to the next file*/
            currentFile = currentFile->nextFile;
        }
    }

    /* return NULL to indicate that command not found */
    return NULL;
}