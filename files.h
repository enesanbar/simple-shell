struct file_node {
    char name[256];			    /* User printable name of the function. */
    char path[512];             /* The path of the system command */
    struct file_node *nextCommand;
};

typedef struct file_node FileNode; /* synonym for struct process_node */
typedef FileNode *FileNodePtr; /* synonym for FileNode* */

FileNodePtr files = NULL; /* initialize files */

void insertFiles (FileNodePtr *sPtr, char name[], char path[]) {
    FileNodePtr newPtr;
    FileNodePtr previousPtr;
    FileNodePtr currentPtr;

    newPtr = malloc(sizeof(FileNode));

    if (newPtr != NULL) {
        strcpy(newPtr->name, name);
        strcpy(newPtr->path, path);
        newPtr->nextCommand = NULL;

        previousPtr = NULL;
        currentPtr = *sPtr;

        /* loop to find the correct location to insert the command alphabetically */
        while (currentPtr != NULL && (strcmp(name, currentPtr->name)) > 0) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextCommand;
        }

        /* insert the command at the beginning if it's initially empty */
        if (previousPtr == NULL) {
            newPtr->nextCommand = *sPtr;
            *sPtr = newPtr;
        }
        /* insert the new node between previousPtr and currentPtr */
        else {
            previousPtr->nextCommand = newPtr;
            newPtr->nextCommand = currentPtr;
        }
    }

    else {
        printf("The command %s not inserted. No memory available!\n", name);
    }
}

FileNodePtr findFile(FileNodePtr currentFile, char *name) {
    /* if list is empty */
    if (currentFile == NULL) {
        printf("Command list is empty");
    }

    else {
        while ( currentFile != NULL ) {
            if (strcmp(currentFile->name, name) == 0)
                return currentFile;

            currentFile = currentFile->nextCommand;
        }

    }

    return NULL;
}

void printSystemFiles(FileNodePtr currentFile) {
    /* if list is empty */
    if (currentFile == NULL) {
        printf("Command list is empty");
    }

    else {
        while ( currentFile != NULL ) {
            printf("%s", currentFile->path);
            printf("\n");
            currentFile = currentFile->nextCommand;
        }

    }
}
