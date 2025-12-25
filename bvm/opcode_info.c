#include <assert.h>
#include "BVM.h"
#include "bvm_pri.h"

static BVM_OpCodeInfo st_opcode_info[] = {
    {"NOP", 0},
    {"PUSH", 1},
    {"PUSH_AUTO", 1},
    {"PUSH_STATIC", 1},
    {"POP_AUTO", 1},
    {"POP_STATIC", 1},
    {"PUSH_AUTO_ADDRESS", 1},
    {"PUSH_BY_STACK", 0},
    {"POP_BY_STACK", 0},
    {"POP", 0},
    {"PUSH_N", 1},
    {"POP_N", 1},
    {"DUPLICATE", 0},
    {"ADD", 0},
    {"SUB", 0},
    {"MUL", 0},
    {"DIV", 0},
    {"MOD", 0},
    {"MINUS", 0},
    {"LEFT_SHIFT", 0},
    {"RIGHT_SHIFT", 0},
    {"EQ", 0},
    {"NE", 0},
    {"GT", 0},
    {"GE", 0},
    {"LT", 0},
    {"LE", 0},
    {"LOGICAL_NOT", 0},
    {"BIT_AND", 0},
    {"BIT_OR", 0},
    {"BIT_XOR", 0},
    {"BIT_NOT", 0},
    {"JUMP", 1},
    {"JUMP_IF_TRUE", 1},
    {"JUMP_IF_FALSE", 1},
    {"JUMP_STACK", 0},
    {"CALL", 0},
    {"SAVE_RETURN_VALUE", 0},
    {"RETURN", 0},
    {"PUSH_RETURN_VALUE", 0},
};
   
BVM_OpCodeInfo *
BVM_get_opcode_info(void)
{
    return st_opcode_info;
}




