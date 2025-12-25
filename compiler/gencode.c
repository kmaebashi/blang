#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "MEM.h"
#include "BVM.h"
#include "compiler.h"

BVM_OpCodeInfo *st_opcode_info;
int st_builtin_function_count;
int *st_buf;
int st_buf_idx;
ParseTree *st_parse_tree;

int st_label_count;
int *st_labels;

int st_pending_push_count;
int *st_pending_pushes;

FunctionDefinition *st_current_function;

static void
gencode(BVM_OpCode code, ...)
{
    va_list argp;
    int i;

/*
    fprintf(stderr, "%4d: %s ", st_buf_idx, st_opcode_info[(int)code].name);
*/
    st_buf[st_buf_idx] = (int)code;
    st_buf_idx++;

    va_start(argp, code);
    for (i = 0; i < st_opcode_info[(int)code].operand_count; i++) {
        int operand = va_arg(argp, int);
/*
        fprintf(stderr, "%d ", operand);
*/
        st_buf[st_buf_idx] = operand;
        st_buf_idx++;
    }
    va_end(argp);
/*
    fprintf(stderr, "\n");
*/
}

static void
set_function_address(char *name)
{
    int i;

    for (i = 0; i < st_parse_tree->static_name_count; i++) {
        if (!strcmp(name, st_parse_tree->static_name[i].name)) {
            st_parse_tree->static_name[i].is_function = TRUE;
            st_parse_tree->static_name[i].address = st_buf_idx;
            return;
        }
    }
    assert(0);
}

static int
get_label(void) {
    int ret;

    ret = st_label_count;
    st_labels = MEM_realloc(st_labels, sizeof(int) * (st_label_count + 1));
    st_label_count++;

    return ret;
}

static void
set_label(int label) {
    st_labels[label] = st_buf_idx;
}

static int
add_pending_push(int code_address) {
    int ret;

    ret = st_pending_push_count;
    st_pending_pushes = MEM_realloc(st_pending_pushes,
                                    sizeof(int) * (st_pending_push_count + 1));
    st_pending_pushes[st_pending_push_count] = code_address;
    st_pending_push_count++;

    return ret;
}

static void gen_expression(Expression *expr);

static void
gen_name_rvalue(Expression *expr) {
    switch (expr->u.name_e.local_name->kind) {
    case EXTERNAL_LOCAL_NAME:
    {
        Boolean is_vec
            = st_parse_tree->static_name[expr->u.name_e.local_name
                                         ->u.ext_ln.static_name_index].is_vec;
        Boolean is_func
            = st_parse_tree->static_name[expr->u.name_e.local_name
                         ->u.ext_ln.static_name_index].is_function;
        if (is_vec || is_func) {
            add_pending_push(st_buf_idx);
            gencode(BVM_PUSH,
                    expr->u.name_e.local_name->u.ext_ln.static_name_index);
        } else {
            gencode(BVM_PUSH_STATIC,
                    expr->u.name_e.local_name->u.ext_ln.static_name_index);
        }
        break;
    }
    case INTERNAL_LOCAL_NAME:
        gencode(BVM_PUSH_STATIC,
                expr->u.name_e.local_name->u.int_ln.static_name_index);
        break;
    case AUTO_LOCAL_NAME:
        if (expr->u.name_e.local_name->u.auto_ln.is_vec) {
            gencode(BVM_PUSH_AUTO_ADDRESS,
                    expr->u.name_e.local_name->u.auto_ln.offset);
        } else {
            gencode(BVM_PUSH_AUTO,
                    expr->u.name_e.local_name->u.auto_ln.offset);
        }
        break;
    default:
        assert(0);
    }
}

static void
gen_string_literal(Expression *expr)
{
    st_parse_tree->string_literal[expr->u.str_e.index].code_address
        = st_buf_idx;
    gencode(BVM_PUSH,expr->u.str_e.index);
}

static void
gen_index_expression(Expression *expr, Boolean is_lvalue)
{
    gen_expression(expr->u.index_e.vector);
    gen_expression(expr->u.index_e.index);
    gencode(BVM_ADD);
    if (!is_lvalue) {
        gencode(BVM_PUSH_BY_STACK);
    }
}

static int gen_args(Argument *args, int count)
{
    if (args == NULL) {
        return count;
    }
    count = gen_args(args->next, count);
    gen_expression(args->expr);
    count++;

    return count;
}

static void
gen_function_call_expression(Expression *expr)
{
    int arg_count = 0;

    arg_count = gen_args(expr->u.func_call_e.args, arg_count);
    gencode(BVM_PUSH, arg_count);
    gen_expression(expr->u.func_call_e.func);
    gencode(BVM_CALL);
    gencode(BVM_POP_N, arg_count + 1);
    gencode(BVM_PUSH_RETURN_VALUE);
}


static void
gen_indirection_expression(Expression *expr, Boolean is_lvalue)
{
    gen_expression(expr->u.unary_e.operand);
    if (!is_lvalue) {
        gencode(BVM_PUSH_BY_STACK);
    }
}

static void
gen_push_name_address(Expression *expr)
{
    switch (expr->u.name_e.local_name->kind) {
    case EXTERNAL_LOCAL_NAME:
        add_pending_push(st_buf_idx);
        gencode(BVM_PUSH,
                expr->u.name_e.local_name->u.ext_ln.static_name_index);
        break;
    case INTERNAL_LOCAL_NAME:
        add_pending_push(st_buf_idx);
        gencode(BVM_PUSH,
                expr->u.name_e.local_name->u.int_ln.static_name_index);
        break;
    case AUTO_LOCAL_NAME:
        gencode(BVM_PUSH_AUTO_ADDRESS,
                expr->u.name_e.local_name->u.auto_ln.offset);
        break;
    default:
        break;
    }
}

static void
gen_address_expression(Expression *expr)
{
    if (expr->kind == NAME_EXPRESSION) {
        gen_push_name_address(expr);
    } else if (expr->kind == INDEX_EXPRESSION) {
        gen_index_expression(expr, TRUE);
    } else if (expr->kind == UNARY_EXPRESSION
               && expr->u.unary_e.operator == INDIRECTION_OPERATOR) {
        gen_indirection_expression(expr, TRUE);
    } else {
        assert(0);
    }
}

static void
gen_assign_to_name(Expression *expr)
{
    switch (expr->u.name_e.local_name->kind) {
    case EXTERNAL_LOCAL_NAME:
        gencode(BVM_POP_STATIC,
                expr->u.name_e.local_name->u.ext_ln.static_name_index);
        break;
    case INTERNAL_LOCAL_NAME:
        gencode(BVM_POP_STATIC,
                expr->u.name_e.local_name->u.int_ln.static_name_index);
        break;
    case AUTO_LOCAL_NAME:
        if (expr->u.name_e.local_name->u.auto_ln.is_vec) {
            gencode(BVM_PUSH_AUTO_ADDRESS,
                    expr->u.name_e.local_name->u.auto_ln.offset);
        } else {
            gencode(BVM_POP_AUTO,
                    expr->u.name_e.local_name->u.auto_ln.offset);
        }
        break;
    default:
        assert(0);
    }
}

static void
gen_assign_to_index(Expression *expr)
{
    gen_index_expression(expr, TRUE);
    gencode(BVM_POP_BY_STACK);
}

static void
gen_assign_to_indirection(Expression *expr)
{
    gen_indirection_expression(expr, TRUE);
    gencode(BVM_POP_BY_STACK);
}

static void
gen_assign_to(Expression *expr)
{
    if (expr->kind == NAME_EXPRESSION) {
        gen_assign_to_name(expr);
    } else if (expr->kind == INDEX_EXPRESSION) {
        gen_assign_to_index(expr);
    } else if (expr->kind == UNARY_EXPRESSION
               && expr->u.unary_e.operator == INDIRECTION_OPERATOR) {
        gen_assign_to_indirection(expr);
    } else {
        assert(0);
    }
}

static void
gen_pre_inc_dec_expression(Expression *expr, Boolean is_toplevel)
{
    gen_expression(expr->u.unary_e.operand);
    gencode(BVM_PUSH, 1);
    if (expr->u.unary_e.operator == PRE_INC_OPERATOR) {
        gencode(BVM_ADD);
    } else {
        assert(expr->u.unary_e.operator == PRE_DEC_OPERATOR);
        gencode(BVM_SUB);
    }
    if (!is_toplevel) {
        gencode(BVM_DUPLICATE);
    }
    gen_assign_to(expr->u.unary_e.operand);
}

static void
gen_post_inc_dec_expression(Expression *expr, Boolean is_toplevel)
{
    gen_expression(expr->u.unary_e.operand);
    if (!is_toplevel) {
        gencode(BVM_DUPLICATE);
    }
    gencode(BVM_PUSH, 1);
    if (expr->u.unary_e.operator == POST_INC_OPERATOR) {
        gencode(BVM_ADD);
    } else {
        assert(expr->u.unary_e.operator == POST_INC_OPERATOR);
        gencode(BVM_SUB);
    }
    gen_assign_to(expr->u.unary_e.operand);
}


static void
gen_unary_expression(Expression *expr)
{
    switch (expr->u.unary_e.operator) {
    case INDIRECTION_OPERATOR:
        gen_indirection_expression(expr, FALSE);
        break;
    case ADDRESS_OPERATOR:
        gen_address_expression(expr->u.unary_e.operand);
        break;
    case MINUS_OPERATOR:
        gencode(BVM_MINUS);
        break;
    case LOGICAL_NOT_OPERATOR:
        gencode(BVM_LOGICAL_NOT);
        break;
    case PRE_INC_OPERATOR: /* FALLTHROUGH */
    case PRE_DEC_OPERATOR:
        gen_pre_inc_dec_expression(expr, TRUE);
        break;
    case POST_INC_OPERATOR: /* FALLTHROUGH */
    case POST_DEC_OPERATOR:
        gen_post_inc_dec_expression(expr, TRUE);
        break;
    case BIT_NOT_OPERATOR:
        gencode(BVM_BIT_NOT);
        break;
    default:
        assert(0);
    }
}

static void
gen_normal_assignment(Expression *expr, Boolean is_toplevel)
{
    gen_expression(expr->u.binary_e.right);
    if (!is_toplevel) {
        gencode(BVM_DUPLICATE);
    }
    gen_assign_to(expr->u.binary_e.left);
}

static void
gen_compound_assignment(Expression *expr, BVM_OpCode opcode,
                        Boolean is_toplevel)
{
    gen_expression(expr->u.binary_e.left);
    gen_expression(expr->u.binary_e.right);
    gencode(opcode);
    if (!is_toplevel) {
        gencode(BVM_DUPLICATE);
    }
    gen_assign_to(expr->u.binary_e.left);
}

static void
gen_assignment_expression(Expression *expr, Boolean is_toplevel)
{
    if (expr->u.binary_e.operator == ASSIGN_OPERATOR) {
        gen_normal_assignment(expr, is_toplevel);
    } else {
        BVM_OpCode code;
        BinaryOperator op;

        op = expr->u.binary_e.operator;
        if (op == ADD_ASSIGN_OPERATOR) {
            code = BVM_ADD;
        } else if (op == SUB_ASSIGN_OPERATOR) {
            code = BVM_SUB;
        } else if (op == MUL_ASSIGN_OPERATOR) {
            code = BVM_MUL;
        } else if (op == DIV_ASSIGN_OPERATOR) {
            code = BVM_DIV;
        } else if (op == MOD_ASSIGN_OPERATOR) {
            code = BVM_MOD;
        } else if (op == LEFT_SHIFT_ASSIGN_OPERATOR) {
            code = BVM_LEFT_SHIFT;
        } else if (op == RIGHT_SHIFT_ASSIGN_OPERATOR) {
            code = BVM_RIGHT_SHIFT;
        } else if (op == LT_ASSIGN_OPERATOR) {
            code = BVM_LT;
        } else if (op == LE_ASSIGN_OPERATOR) {
            code = BVM_LE;
        } else if (op == GT_ASSIGN_OPERATOR) {
            code = BVM_GT;
        } else if (op == GE_ASSIGN_OPERATOR) {
            code = BVM_GE;
        } else if (op == EQ_ASSIGN_OPERATOR) {
            code = BVM_EQ;
        } else if (op == NE_ASSIGN_OPERATOR) {
            code = BVM_NE;
        } else if (op == BIT_AND_ASSIGN_OPERATOR) {
            code = BVM_BIT_AND;
        } else if (op == BIT_XOR_ASSIGN_OPERATOR) {
            code = BVM_BIT_XOR;
        } else if (op == BIT_OR_ASSIGN_OPERATOR) {
            code = BVM_BIT_OR;
        } else {
            assert(0);
        }
        gen_compound_assignment(expr, code, FALSE);
    }
}

static void
gen_simple_binary(Expression *expr, BVM_OpCode opcode)
{
    gen_expression(expr->u.binary_e.left);
    gen_expression(expr->u.binary_e.right);
    gencode(opcode);
}

static void
gen_binary_expression(Expression *expr)
{
    switch (expr->u.binary_e.operator) {
    case MUL_OPERATOR:
        gen_simple_binary(expr, BVM_MUL);
        break;
    case DIV_OPERATOR:
        gen_simple_binary(expr, BVM_DIV);
        break;
    case MOD_OPERATOR:
        gen_simple_binary(expr, BVM_MOD);
        break;
    case ADD_OPERATOR:
        gen_simple_binary(expr, BVM_ADD);
        break;
    case SUB_OPERATOR:
        gen_simple_binary(expr, BVM_SUB);
        break;
    case LEFT_SHIFT_OPERATOR:
        gen_simple_binary(expr, BVM_LEFT_SHIFT);
        break;
    case RIGHT_SHIFT_OPERATOR:
        gen_simple_binary(expr, BVM_RIGHT_SHIFT);
        break;
    case LT_OPERATOR:
        gen_simple_binary(expr, BVM_LT);
        break;
    case LE_OPERATOR:
        gen_simple_binary(expr, BVM_LE);
        break;
    case GT_OPERATOR:
        gen_simple_binary(expr, BVM_GT);
        break;
    case GE_OPERATOR:
        gen_simple_binary(expr, BVM_GE);
        break;
    case EQ_OPERATOR:
        gen_simple_binary(expr, BVM_EQ);
        break;
    case NE_OPERATOR:
        gen_simple_binary(expr, BVM_NE);
        break;
    case BIT_AND_OPERATOR:
        gen_simple_binary(expr, BVM_BIT_AND);
        break;
    case BIT_XOR_OPERATOR:
        gen_simple_binary(expr, BVM_BIT_XOR);
        break;
    case BIT_OR_OPERATOR:
        gen_simple_binary(expr, BVM_BIT_OR);
        break;
    case ASSIGN_OPERATOR: /* FALLTHROUGH */
    case ADD_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case SUB_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case MUL_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case DIV_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case MOD_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case LEFT_SHIFT_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case RIGHT_SHIFT_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case LT_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case LE_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case GT_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case GE_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case EQ_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case NE_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case BIT_AND_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case BIT_XOR_ASSIGN_OPERATOR: /* FALLTHROUGH */
    case BIT_OR_ASSIGN_OPERATOR: /* FALLTHROUGH */
        gen_assignment_expression(expr, FALSE);
        break;
    default:
        assert(0);
    }
}

static void
gen_conditional_expression(Expression *expr)
{
    int false_label = get_label();
    int end_label = get_label();

    gen_expression(expr->u.cond_e.cond);
    gencode(BVM_JUMP_IF_FALSE, false_label);
    gen_expression(expr->u.cond_e.expr1);
    gencode(BVM_JUMP, end_label);
    set_label(false_label);
    gen_expression(expr->u.cond_e.expr2);
    set_label(end_label);
}

static void
gen_expression(Expression *expr)
{
    switch (expr->kind) {
    case NAME_EXPRESSION:
        gen_name_rvalue(expr);
        break;
    case INTEGER_LITERAL:
        gencode(BVM_PUSH, expr->u.int_e.int_value);
        break;
    case CHARS_LITERAL:
        gencode(BVM_PUSH, expr->u.chars_e.chars);
        break;
    case STRING_LITERAL:
        gen_string_literal(expr);
        break;
    case INDEX_EXPRESSION:
        gen_index_expression(expr, FALSE);
        break;
    case FUNCTION_CALL_EXPRESSION:
        gen_function_call_expression(expr);
        break;
    case UNARY_EXPRESSION:
        gen_unary_expression(expr);
        break;
    case BINARY_EXPRESSION:
        gen_binary_expression(expr);
        break;
    case CONDITIONAL_EXPRESSION:
        gen_conditional_expression(expr);
        break;
    default:
        assert(0);
    }
}

static void gen_statement(Statement *stmt);

static void
gen_labeled_statement(Statement *stmt)
{
    int idx;
    IVal *address;
    Constant *constant;

    assert(stmt->u.label_s.local_name->kind == INTERNAL_LOCAL_NAME);
    idx = stmt->u.label_s.local_name->u.int_ln.static_name_index;

    address = bcp_alloc_ival(CONSTANT_IVAL);
    constant = bcp_malloc(sizeof(Constant));
    constant->kind = INT_CONSTANT;
    constant->u.int_value = st_buf_idx;
    address->u.c.constant = constant;
    st_parse_tree->static_name[idx].initializer = address;

    gen_statement(stmt->u.label_s.following);
}

static void
gen_case_statement(Statement *stmt)
{
    set_label(stmt->u.case_s.this_case->label);
    gen_statement(stmt->u.case_s.following);
}

static void
gen_default_statement(Statement *stmt)
{
    set_label(stmt->u.default_s.this_case->label);
    gen_statement(stmt->u.default_s.following);
}

static void
gen_compound_statement(Statement *stmt)
{
    Statement *pos;

    for (pos = stmt->u.compound_s.stmt; pos != NULL; pos = pos->next) {
        gen_statement(pos);
    }
}

static void
gen_if_statement(Statement *stmt)
{
    int false_label = get_label();
    int end_label;

    gen_expression(stmt->u.if_s.cond);
    gencode(BVM_JUMP_IF_FALSE, false_label);

    gen_statement(stmt->u.if_s.then_clause);
    set_label(false_label);
    if (stmt->u.if_s.else_clause) {
        end_label = get_label();
        gencode(BVM_JUMP, end_label);
        gen_statement(stmt->u.if_s.else_clause);
        set_label(end_label);
    }
}

static void
gen_while_statement(Statement *stmt)
{
    int start_label = get_label();
    int end_label = get_label();

    set_label(start_label);
    gen_expression(stmt->u.while_s.cond);
    gencode(BVM_JUMP_IF_FALSE, end_label);
    stmt->u.while_s.end_label = end_label;
    gen_statement(stmt->u.while_s.stmt);
    gencode(BVM_JUMP, start_label);
    set_label(end_label);
}

static void
gen_switch_statement(Statement *stmt)
{
    CaseClause *cc;
    int next_label;

    gen_expression(stmt->u.switch_s.value);
    for (cc = stmt->u.switch_s.case_list; cc != NULL; cc = cc->next) {
        gencode(BVM_DUPLICATE);
        cc->label = get_label();
        assert(cc->stmt->kind == CASE_STATEMENT);
        gen_expression(cc->stmt->u.case_s.expr);
        gencode(BVM_EQ);
        next_label = get_label();
        gencode(BVM_JUMP_IF_FALSE, next_label);
        gencode(BVM_POP);
        gencode(BVM_JUMP, cc->label);
        set_label(next_label);

    }
    gencode(BVM_POP);
    if (stmt->u.switch_s.default_case) {
        int default_label = get_label();
        stmt->u.switch_s.default_case->label = default_label;
        gencode(BVM_JUMP, default_label);
    }
    stmt->u.switch_s.end_label = get_label();
    gen_statement(stmt->u.switch_s.stmt);
    set_label(stmt->u.switch_s.end_label);
}

static void
gen_goto_statement(Statement *stmt)
{
    gen_expression(stmt->u.goto_s.value);
    gencode(BVM_JUMP_STACK);
}

static void
gen_break_statement(Statement *stmt)
{
    if (stmt->u.break_s.outer->kind == WHILE_STATEMENT) {
        gencode(BVM_JUMP, stmt->u.break_s.outer->u.while_s.end_label);
    } else {
        assert(stmt->u.break_s.outer->kind == SWITCH_STATEMENT);
        gencode(BVM_JUMP, stmt->u.break_s.outer->u.switch_s.end_label);
    } 
}

static void
gen_return_code(int local_variable_size)
{
    gencode(BVM_SAVE_RETURN_VALUE);
    gencode(BVM_POP_N, local_variable_size);
    gencode(BVM_RETURN);
}

static void
gen_return_statement(Statement *stmt)
{
    if (stmt->u.return_s.value != NULL) {
        gen_expression(stmt->u.return_s.value);
    } else {
        gencode(BVM_PUSH, 0);
    }
    gen_return_code(st_current_function->local_variable_size);
}

static void
gen_expression_statement(Statement *stmt)
{
    Expression *expr = stmt->u.expr_s.expr;

    if (expr->kind == BINARY_EXPRESSION
        && is_assign_operator(expr->u.binary_e.operator)) {
        gen_assignment_expression(expr, TRUE);
    } else if (expr->kind == UNARY_EXPRESSION
               && (expr->u.unary_e.operator == PRE_INC_OPERATOR
                   || expr->u.unary_e.operator == PRE_DEC_OPERATOR)) {
        gen_pre_inc_dec_expression(expr, TRUE);
    } else if (expr->kind == UNARY_EXPRESSION
               && (expr->u.unary_e.operator == POST_INC_OPERATOR
                   || expr->u.unary_e.operator == POST_DEC_OPERATOR)) {
        gen_post_inc_dec_expression(expr, TRUE);
    } else {
        gen_expression(expr);
        gencode(BVM_POP);
    }
}

static void
gen_statement(Statement *stmt)
{
    switch (stmt->kind) {
    case AUTO_STATEMENT:
        gen_statement(stmt->u.auto_s.following);
        break;
    case EXTRN_STATEMENT:
        gen_statement(stmt->u.extrn_s.following);
        break;
    case LABELED_STATEMENT:
        gen_labeled_statement(stmt);
        break;
    case CASE_STATEMENT:
        gen_case_statement(stmt);
        break;
    case DEFAULT_STATEMENT:
        gen_default_statement(stmt);
        break;
    case COMPOUND_STATEMENT:
        gen_compound_statement(stmt);
        break;
    case IF_STATEMENT:
        gen_if_statement(stmt);
        break;
    case WHILE_STATEMENT:
        gen_while_statement(stmt);
        break;
    case SWITCH_STATEMENT:
        gen_switch_statement(stmt);
        break;
    case GOTO_STATEMENT:
        gen_goto_statement(stmt);
        break;
    case BREAK_STATEMENT:
        gen_break_statement(stmt);
        break;
    case RETURN_STATEMENT:
        gen_return_statement(stmt);
        break;
    case NULL_STATEMENT:
        /* nothing to do */
        break;
    case EXPRESSION_STATEMENT:
        gen_expression_statement(stmt);
        break;
    default:
        assert(0);
    }
}

static void
gen_functions(Definition *head)
{
    Definition *def;

    for (def = head; def != NULL; def = def->next) {
        if (def->kind != FUNCTION_DEFINITION) {
            continue;
        }
        st_current_function = &def->u.func_def;
        set_function_address(def->u.func_def.name);
        gencode(BVM_PUSH_N, def->u.func_def.local_variable_size);
        gen_statement(def->u.func_def.stmt);
        gencode(BVM_PUSH, 0);
        gen_return_code(def->u.func_def.local_variable_size);
        st_current_function = NULL;
    }
}

static void
fix_string_literal(int def_count, StringLiteralDef *def)
{
    int def_idx = 0;
    int i;
    int str_len; /* including '*0' */

    for (def_idx = 0; def_idx < def_count; def_idx++) {
        signed char *target = (signed char*)&st_buf[st_buf_idx];
        def[def_idx].str_address = st_buf_idx;

        for (i = 0; i < def[def_idx].str_literal.length; i++) {
            target[i] = def[def_idx].str_literal.str[i];
        }
        target[i] = BVM_EOT;
        str_len = def[def_idx].str_literal.length + 1;
        st_buf_idx += ((str_len + sizeof(int) - 1)) / sizeof(int);

        if (def[def_idx].in_code) {
            assert(st_buf[def[def_idx].code_address] == (int)BVM_PUSH);
            st_buf[def[def_idx].code_address + 1] = def[def_idx].str_address;
        }
    }
}

static void
set_initial_value(int address, IVal *ival, StaticName *sn, int current_idx,
                  StringLiteralDef *sld)
{
    switch (ival->kind) {
    case CONSTANT_IVAL:
        switch (ival->u.c.constant->kind) {
        case INT_CONSTANT:
            st_buf[address] = ival->u.c.constant->u.int_value;
            break;
        case CHARS_CONSTANT:
            ((unsigned int*)st_buf)[address] = ival->u.c.constant->u.chars;
            break;
        case STRING_CONSTANT:
            st_buf[address]
                = sld[ival->u.c.string_literal_index].str_address;
            break;
        }
        break;
    case NAME_IVAL:
        st_buf[address] = sn[ival->u.n.static_name_index].address;
        break;
    default:
        assert(0);
    }
}

static void
fix_static_name_address(int sn_count, StaticName *sn, StringLiteralDef *sld)
{
    int sn_idx;
    IVal *pos;

    for (sn_idx = 0;
         sn_idx < st_builtin_function_count; sn_idx++) {
        sn[sn_idx].address = sn_idx;
    }

    for (; sn_idx < sn_count; sn_idx++) {
        if (sn[sn_idx].is_function) {
            continue;
        }
        sn[sn_idx].address = st_buf_idx;
        if (sn[sn_idx].is_vec) {
            int vec_idx = 0;
            memset(&st_buf[st_buf_idx], 0, sn[sn_idx].vec_size * sizeof(int));
            for (pos = sn[sn_idx].initializer; pos != NULL; pos = pos->next) {
                set_initial_value(st_buf_idx + vec_idx, pos,
                                  sn, sn_idx, sld);
                vec_idx++;
            }
            st_buf_idx += sn[sn_idx].vec_size;
        } else {
            st_buf[st_buf_idx] = 0;
            if (sn[sn_idx].initializer != NULL) {
                set_initial_value(st_buf_idx, sn[sn_idx].initializer,
                                  sn, sn_idx, sld);
            }
            st_buf_idx++;
        }
    }
}

static void
check_static_name(int sn_count, StaticName *sn)
{
    int i;

    for (i = 0; i < sn_count; i++) {
        if (!sn[i].defined) {
            bcp_compile_error(NAME_NOT_DEFINED_ERR, 0, sn[i].name);
        }
    }
}

static void
fix_pending_pushes(void)
{
    int i;
    int static_name_idx;

    for (i = 0; i < st_pending_push_count; i++) {
        assert(st_buf[st_pending_pushes[i]] == (int)BVM_PUSH);
        static_name_idx = st_buf[st_pending_pushes[i] + 1];

        st_buf[st_pending_pushes[i] + 1]
            = st_parse_tree->static_name[static_name_idx].address;
    }
}

static void
fix_code(void)
{
    int code_pos;
    int static_name_index;
    int label_index;

    for (code_pos = 0; code_pos < st_parse_tree->code_max; ) {
        BVM_OpCodeInfo *op = &st_opcode_info[st_buf[code_pos]];

        if (st_buf[code_pos] == BVM_PUSH_STATIC
            || st_buf[code_pos] == BVM_POP_STATIC) {
            static_name_index = st_buf[code_pos + 1];
            st_buf[code_pos + 1]
                = st_parse_tree->static_name[static_name_index].address;
        } else if (st_buf[code_pos] == BVM_JUMP
                   || st_buf[code_pos] == BVM_JUMP_IF_TRUE
                   || st_buf[code_pos] == BVM_JUMP_IF_FALSE) {
            label_index = st_buf[code_pos + 1];
            st_buf[code_pos + 1] = st_labels[label_index];
        }
        code_pos += op->operand_count + 1;
    }
}

static void
initialize_buf(void)
{
    int i;

    st_buf = MEM_malloc(sizeof(int) * BVM_MEMORY_SIZE);
    st_buf_idx = st_builtin_function_count;
    for (i = 0; i < st_builtin_function_count; i++) {
        st_buf[i] = (int)BVM_NOP;
    }
}

int *
bcp_generate_code(ParseTree *parse_tree)
{
    st_opcode_info = BVM_get_opcode_info();
    BVM_get_builtin_functions(&st_builtin_function_count);

    st_parse_tree = parse_tree;
    
    initialize_buf();

    if (st_labels != NULL) {
        MEM_free(st_labels);
    }
    st_labels = NULL;
    st_label_count = 0;

    gen_functions(parse_tree->def_head);
    parse_tree->code_max = st_buf_idx;

    fix_string_literal(parse_tree->string_literal_count,
                       parse_tree->string_literal);
    fix_static_name_address(parse_tree->static_name_count,
                            parse_tree->static_name,
                            parse_tree->string_literal);
    check_static_name(parse_tree->static_name_count,
                      parse_tree->static_name);
    fix_pending_pushes();
    fix_code();

    return st_buf;
}
