#ifndef BVM_H_INCLUDED
#define BVM_H_INCLUDED

#define BVM_RETURN_INFO_SIZE (2)
#define BVM_MEMORY_SIZE (65536)

typedef enum {
    BVM_NOP,
    BVM_PUSH,
    BVM_PUSH_STACK,
    BVM_PUSH_STATIC,
    BVM_POP_STACK,
    BVM_POP_STATIC,
    BVM_PUSH_STACK_ADDRESS,
    BVM_POP,
    BVM_DUPLICATE,
    BVM_ADD,
    BVM_SUB,
    BVM_MUL,
    BVM_DIV,
    BVM_MOD,
    BVM_MINUS,
    BVM_EQ,
    BVM_NE,
    BVM_GT,
    BVM_GE,
    BVM_LT,
    BVM_LE,
    BVM_BIT_AND,
    BVM_BIT_OR,
    BVM_BIT_XOR,
    BVM_BIT_NOT,
    BVM_JUMP,
    BVM_JUMP_IF_TRUE,
    BVM_JUMP_IF_FALSE,
    BVM_CALL,
    BVM_RETURN
} BVM_OpCode;

typedef struct {
    char *name;
    int operand_count;
} BVM_OpCodeInfo;

BVM_OpCodeInfo *BVM_get_opcode_info(void);

#endif /* BVM_H_INCLUDED */
