struct builtin_node {
    char *name;
    int *(*function) (char *args[]);
    struct builtin_node *nextBuiltIn;
};

typedef struct builtin_node BuiltInNode;
typedef BuiltInNode *BuiltinNodePtr;

void insertIntoBuiltins(BuiltinNodePtr *headPtr, char *name, int *(*function) (char *args[]));

BuiltinNodePtr findBuiltIn(BuiltinNodePtr builtinHead, char *name);