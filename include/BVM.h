#ifndef BVM_H_INCLUDED
#define BVM_H_INCLUDED

#define BVM_RETURN_INFO_SIZE (2)
#define BVM_MEMORY_SIZE (65536)

typedef enum {
    BVM_NOP,
    BVM_PUSH,
    BVM_PUSH_AUTO,
    BVM_PUSH_STATIC,
    BVM_POP_AUTO,
    BVM_POP_STATIC,
    BVM_PUSH_AUTO_ADDRESS,
    BVM_PUSH_BY_STACK,
    BVM_POP_BY_STACK,
    BVM_POP,
    BVM_PUSH_N,
    BVM_POP_N,
    BVM_DUPLICATE,
    BVM_ADD,
    BVM_SUB,
    BVM_MUL,
    BVM_DIV,
    BVM_MOD,
    BVM_MINUS,
    BVM_LEFT_SHIFT,
    BVM_RIGHT_SHIFT,
    BVM_EQ,
    BVM_NE,
    BVM_GT,
    BVM_GE,
    BVM_LT,
    BVM_LE,
    BVM_LOGICAL_NOT,
    BVM_BIT_AND,
    BVM_BIT_OR,
    BVM_BIT_XOR,
    BVM_BIT_NOT,
    BVM_JUMP,
    BVM_JUMP_IF_TRUE,
    BVM_JUMP_IF_FALSE,
    BVM_JUMP_STACK,
    BVM_CALL,
    BVM_SAVE_RETURN_VALUE,
    BVM_RETURN,
    BVM_PUSH_RETURN_VALUE
} BVM_OpCode;

typedef struct {
    char *name;
    int operand_count;
} BVM_OpCodeInfo;

BVM_OpCodeInfo *BVM_get_opcode_info(void);
char **BVM_get_builtin_functions(int *count);
int BVM_execute(int *memory, int main_address);

#endif /* BVM_H_INCLUDED */
