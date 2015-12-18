struct file_node {
    char *name;
    char *path;
    struct file_node *nextFile;
};

typedef struct file_node FileNode; /* synonym for struct process_node */
typedef FileNode *FileNodePtr; /* synonym for FileNode* */

void insertFiles (FileNodePtr *sPtr, char name[], char path[]);

FileNodePtr findFile(FileNodePtr currentFile, char *name);