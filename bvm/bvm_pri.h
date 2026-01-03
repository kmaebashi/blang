#ifndef BVM_PRI_H_INCLUDED
#define BVM_PRI_H_INCLUDED
typedef struct {
    char *name;
    int (*func)(int argc, int *args, int *memory);
} BuiltinFunction;

extern BuiltinFunction bvm_builtin_functions[];

typedef struct {
    int size;
    int next;
} FreeBlock;

#define HEAP_HEADER_SIZE ((sizeof(FreeBlock) + sizeof(int) - 1) / sizeof(int))
#define BVM_NULL (-1)
#define HEAP_ALLOC_SIZE (1024)

/* buitin_func.c */
int bvm_get_builtin_function_count(void);
void bvm_init_builtin_function(void);

/* execute.c */
int bvm_get_current_arg_count(void);
int bvm_get_heap_start_address(void);
int bvm_get_heap_end_address(void);
void bvm_set_heap_end_address(int new_end_address);

#endif /* BVM_PRI_H_INCLUDED */
