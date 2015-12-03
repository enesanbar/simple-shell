#define RUNNING 1
#define FINISHED 0
#define PROCESS 1
#define INDEX 0

/* struct to construct a list of processes. */
struct process_node {
    int index;
    pid_t process_id;
    char **args;
    int number_of_args;
    int is_running;
    struct process_node *nextProcess;
};

typedef struct process_node ProcessNode; /* synonym for struct process_node */
typedef ProcessNode *ProcessNodePtr; /* synonym for ProcessNode* */

ProcessNodePtr processHead = NULL; /* initialize headPtr */
ProcessNodePtr processTail = NULL; /* initialize tailPtr */

int isProcessesEmpty(ProcessNodePtr headPtr) {
    return headPtr == NULL;
}

void insertIntoProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr,
                          long int process_id, char **args, int number_of_args) {
    ProcessNodePtr newPtr; /* pointer to a new path */
    int i;

    /* allocate memory for the new node */
    newPtr = malloc( sizeof(ProcessNode) );

    if (newPtr != NULL) {
        newPtr->process_id = process_id;    /* process id of the child */
        newPtr->is_running = RUNNING;       /* it's running */
        newPtr->nextProcess = NULL;

        newPtr->number_of_args = number_of_args;
        newPtr->args = malloc(sizeof(char*) * (number_of_args));
        for (i = 0; i < number_of_args; i++) {
            newPtr->args[i] = malloc(strlen(args[i]) + 1);
            strcpy(newPtr->args[i], args[i]);
        }

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
    if (id == (*headPtr)->process_id) {
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

ProcessNodePtr findProcess(ProcessNodePtr processes, int id, int id_type) {
    /* if queue is empty */
    if (processes == NULL) {
        return NULL;
    }

    if (id_type == PROCESS){
        while (processes != NULL) {
            if (processes->process_id == id) return processes;
            processes = processes->nextProcess;
        }
    }

    else if (id_type == INDEX) {
        while (processes != NULL) {
            if (processes->index == id) return processes;
            processes = processes->nextProcess;
        }
    }

    return NULL;
}