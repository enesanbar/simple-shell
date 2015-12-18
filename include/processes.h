#define RUNNING 1
#define FINISHED 0
#define PROCESS_ID 1
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

void insertIntoProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr,
                         pid_t process_id, char **args, int number_of_args);

long int removeFromProcesses(ProcessNodePtr *headPtr, ProcessNodePtr *tailPtr, long int id);

ProcessNodePtr findProcess(ProcessNodePtr processes, int id, int id_type);