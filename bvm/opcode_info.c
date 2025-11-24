#include "BVM.h"

static BVM_OpCodeInfo st_opcode_info[] = {
    {"BVM_NOP", 0},
    {"BVM_PUSH", 1},
    {"BVM_PUSH_STACK", 1},
    {"BVM_PUSH_STATIC", 1},
    {"BVM_POP_STACK", 1},
    {"BVM_POP_STATIC", 1},
    {"BVM_PUSH_STACK_ADDRESS", 1},
    {"BVM_POP", 0},
    {"BVM_DUPLICATE", 0},
    {"BVM_ADD", 0},
    {"BVM_SUB", 0},
    {"BVM_MUL", 0},
    {"BVM_DIV", 0},
    {"BVM_MOD", 0},
    {"BVM_MINUS", 0},
    {"BVM_EQ", 0},
    {"BVM_NE", 0},
    {"BVM_GT", 0},
    {"BVM_GE", 0},
    {"BVM_LT", 0},
    {"BVM_LE", 0},
    {"BVM_BIT_AND", 0},
    {"BVM_BIT_OR", 0},
    {"BVM_BIT_XOR", 0},
    {"BVM_BIT_NOT", 0},
    {"BVM_JUMP", 0},
    {"BVM_JUMP_IF_TRUE", 1},
    {"BVM_JUMP_IF_FALSE", 1},
    {"BVM_CALL", 1},
    {"BVM_RETURN", 0},
};
   
BVM_OpCodeInfo *
BVM_get_opcode_info(void)
{
    return st_opcode_info;
}
