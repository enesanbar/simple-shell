#define RUNNING 1
#define FINISHED 0

/* struct to construct a list of processes. */
struct process_node {
    int index;
    long int process_id;
    char *name;
    int is_running;
    int is_displayed_once;
    struct process_node *nextProcess;
};

typedef struct process_node ProcessNode; /* synonym for struct process_node */
typedef ProcessNode *ProcessNodePtr; /* synonym for ProcessNode* */

ProcessNodePtr headProcesses = NULL; /* initialize headPtr */
ProcessNodePtr tailProcesses = NULL; /* initialize tailPtr */

int isProcessesEmpty(ProcessNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr,
                          long int process_id, char *name) {
    ProcessNodePtr newPtr; /* pointer to a new path */

    /* allocate memory for the new node */
    newPtr = malloc( sizeof(ProcessNode) );

    if (newPtr != NULL) {
        newPtr->process_id = process_id;    /* process id of the child */
        newPtr->name = name;                /* name of the running program */
        newPtr->is_running = RUNNING;       /* it's running */
        newPtr->is_displayed_once = false;  /* it hasn't been displayed on the screen after termination */
        newPtr->nextProcess = NULL;

        /* if empty, insert the node at head */
        if (isProcessesEmpty(*headPtr)) {
            newPtr->index = 1;              /* first node has the 1st index*/
            *headPtr = newPtr;
        }
        /* otherwise, insert it to the tail */
        else {
            newPtr->index = (*tailPtr)->index + 1;
            (*tailPtr)->nextProcess = newPtr;
        }

        *tailPtr = newPtr;
    } else {
        printf("%ld not inserted. No memory available.\n", process_id);
    }
}

long int removeFromProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr, long int id) {
    ProcessNodePtr previousPtr;
    ProcessNodePtr currentPtr;
    ProcessNodePtr tempPtr;

    /* if the process to terminate is the first one */
    if (id == (*headPtr)->index) {
        tempPtr = *headPtr;
        *headPtr = (*headPtr)->nextProcess;
        free(tempPtr);
        return id;
    }

    else {
        previousPtr = *headPtr;
        currentPtr = (*headPtr)->nextProcess;

        /* loop through the processes to find the correct location */
        while (currentPtr != NULL && currentPtr->process_id != id) {
            previousPtr = currentPtr;
            currentPtr = currentPtr->nextProcess;
        }

        /* delete node at currentPtr */
        if (currentPtr != NULL) {
            tempPtr = currentPtr;
            previousPtr->nextProcess = currentPtr->nextProcess;
            free(tempPtr);
            return id;
        }
    }

    return 0;
}