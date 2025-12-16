#ifndef BVM_PRI_H_INCLUDED
#define BVM_PRI_H_INCLUDED
typedef struct {
    char *name;
    int (*func)(int *args, int *memory);
} BuiltinFunction;

extern BuiltinFunction bvm_builtin_functions[];

int bvm_fun_printf(int *args, int *memory);
/*
int bvm_fun_print(int *args, int *memory);
int bvm_fun_print_s(int *args, int *memory);
*/
int bvm_get_builtin_function_count(void);

#endif /* BVM_PRI_H_INCLUDED */
