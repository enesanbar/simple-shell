/* Struct to construct a list of paths. */
struct path_node {
    char *path;
    struct path_node *nextPath;
};

typedef struct path_node PathNode; /* synonym for struct path_node */
typedef PathNode *PathNodePtr; /* synonym for PathNode* */

void insertIntoPath( PathNodePtr *headPtr, PathNodePtr *tailPtr, char *path );