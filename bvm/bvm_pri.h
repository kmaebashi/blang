#ifndef BVM_PRI_H_INCLUDED
#define BVM_PRI_H_INCLUDED
typedef struct {
    char *name;
    int (*func)(int argc, int *args, int *memory);
} BuiltinFunction;

extern BuiltinFunction bvm_builtin_functions[];

/*
int bvm_fun_print(int *args, int *memory);
int bvm_fun_print_s(int *args, int *memory);
*/
int bvm_get_builtin_function_count(void);
void bvm_init_builtin_function(void);

/* execute.c */
int bvm_get_current_arg_count(void);

#endif /* BVM_PRI_H_INCLUDED */
