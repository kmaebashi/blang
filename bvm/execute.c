#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "BVM.h"
#include "bvm_pri.h"

static BVM_OpCodeInfo *st_opcode_info;
static int st_builtin_function_count;

static int
call_builtin_function(int func_idx, int sp, int *memory)
{
    return bvm_builtin_functions[func_idx].func(&memory[sp + 1], memory); 
}

static int
execute(int *memory, int main_address)
{
    int pc = main_address;
    int sp = BVM_MEMORY_SIZE - 1;
    int base = sp;
    int return_value;
    
    
    for (;;) {
#if 0
        int i;
        fprintf(stderr, "%4d %5d %s ",
                pc, sp, st_opcode_info[memory[pc]].name);
        for (i = 0; i < st_opcode_info[memory[pc]].operand_count; i++) {
            fprintf(stderr, "%d ", memory[pc + i + 1]);
        }
        fprintf(stderr, "\n");
#endif
        
        switch (memory[pc]) {
        case BVM_NOP:
            break;
        case BVM_PUSH:
            memory[sp] = memory[pc + 1];
            sp--;
            pc += 2;
            break;
        case BVM_PUSH_AUTO:
            memory[sp] = memory[base + memory[pc + 1]];
            sp--;
            pc += 2;
            break;
        case BVM_PUSH_STATIC:
            memory[sp] = memory[memory[pc + 1]];
            sp--;
            pc += 2;
            break;
        case BVM_POP_AUTO:
            memory[base + memory[pc + 1]] = memory[sp + 1];
            sp++;
            pc += 2;
            break;
        case BVM_POP_STATIC:
            memory[memory[pc + 1]] = memory[sp + 1];
            sp++;
            pc += 2;
            break;
        case BVM_PUSH_AUTO_ADDRESS:
            memory[sp] = base + memory[pc + 1];
            sp--;
            pc += 2;
            break;
        case BVM_PUSH_BY_STACK:
            memory[sp + 1] = memory[memory[sp + 1]];
            pc++;
            break;
        case BVM_POP_BY_STACK:
            memory[memory[sp + 1]] = memory[sp + 2];
            sp += 2;
            pc++;
            break;
        case BVM_POP:
            sp++;
            pc++;
            break;
        case BVM_PUSH_N:
            sp -= memory[pc + 1];
            base = sp + 1;
            pc += 2;
            break;
        case BVM_POP_N:
            sp += memory[pc + 1];
            pc += 2;
            break;
        case BVM_DUPLICATE:
            memory[sp] = memory[sp + 1];
            sp--;
            pc++;
            break;
        case BVM_ADD:
            memory[sp + 2] = memory[sp + 2] + memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_SUB:
            memory[sp + 2] = memory[sp + 2] - memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_MUL:
            memory[sp + 2] = memory[sp + 2] * memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_DIV:
            memory[sp + 2] = memory[sp + 2] / memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_MOD:
            memory[sp + 2] = memory[sp + 2] % memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_MINUS:
            memory[sp + 1] = -memory[sp + 1];
            pc++;
            break;
        case BVM_LEFT_SHIFT:
            memory[sp + 2] = memory[sp + 2] << memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_RIGHT_SHIFT:
            memory[sp + 2] = memory[sp + 2] >> memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_EQ:
            memory[sp + 2] = memory[sp + 2] == memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_NE:
            memory[sp + 2] = memory[sp + 2] != memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_GT:
            memory[sp + 2] = memory[sp + 2] > memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_GE:
            memory[sp + 2] = memory[sp + 2] >= memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_LT:
            memory[sp + 2] = memory[sp + 2] < memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_LE:
            memory[sp + 2] = memory[sp + 2] <= memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_LOGICAL_NOT:
            memory[sp + 1] = !memory[sp + 1];
            pc++;
            break;
        case BVM_BIT_AND:
            memory[sp + 2] = memory[sp + 2] & memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_BIT_OR:
            memory[sp + 2] = memory[sp + 2] | memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_BIT_XOR:
            memory[sp + 2] = memory[sp + 2] ^ memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_BIT_NOT:
            memory[sp + 1] = ~memory[sp + 1];
            pc++;
            break;
        case BVM_JUMP:
            pc = memory[pc + 1];
            break;
        case BVM_JUMP_IF_TRUE:
            if (memory[sp + 1]) {
                pc = memory[pc + 1];
            } else {
                pc += 2;
            }
            sp++;
            break;
        case BVM_JUMP_IF_FALSE:
            if (!memory[sp + 1]) {
                pc = memory[pc + 1];
            } else {
                pc += 2;
            }
            sp++;
            break;
        case BVM_JUMP_STACK:
            pc = memory[sp + 1];
            sp++;
            break;
        case BVM_CALL:
        {
            int dest_address = memory[sp + 1];
            int ret;
            sp++;
            if (dest_address < st_builtin_function_count) {
                ret = call_builtin_function(dest_address, sp, memory);
                return_value = ret;
                pc++;
            } else {
                sp--;
                memory[sp] = pc;
                memory[sp + 1] = base;
                sp--;
                pc = dest_address;
            }
            break;
        }
        case BVM_SAVE_RETURN_VALUE:
            return_value = memory[sp + 1];
            sp++;
            pc++;
            break;
        case BVM_RETURN:
            if (sp == (BVM_MEMORY_SIZE - 1)) {
                return return_value;
            }
            pc = memory[sp + 1] + 1;
            base = memory[sp + 2];
            sp += 2;
            break;
        case BVM_PUSH_RETURN_VALUE:
            memory[sp] = return_value;
            sp--;
            pc++;
            break;
        default:
            assert(0);
        }
    }
}

int
BVM_execute(int *memory, int main_address)
{
    st_opcode_info = BVM_get_opcode_info();
    st_builtin_function_count = bvm_get_builtin_function_count();

    execute(memory, main_address);

    return 0;
}
