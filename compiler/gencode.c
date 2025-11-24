#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include "MEM.h"
#include "BVM.h"
#include "compiler.h"

BVM_OpCodeInfo *st_opcode_info;
int *st_buf;
int st_buf_idx;
ParseTree *st_parse_tree;

int st_label_count;
int *st_labels;

static void
gencode(BVM_OpCode code, ...)
{
    va_list argp;
    int i;

    st_buf[st_buf_idx] = (int)code;
    st_buf_idx++;

    va_start(argp, code);
    for (i = 0; i < st_opcode_info[(int)code].operand_count; i++) {
	st_buf[st_buf_idx] = va_arg(argp, int);
	st_buf_idx++;
    }
    va_end(argp);
}

static void
set_static_address(char *name)
{
    int i;

    for (i = 0; i < st_parse_tree->static_name_count; i++) {
	if (!strcmp(name, st_parse_tree->static_name[i].name)) {
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

static void gen_expression(Expression *expr);

static void
gen_expression(Expression *expr)
{
    /* BUGBUG */
}

static void
gen_labeled_statement(Statement *stmt)
{
    int idx;

    assert(stmt->u.label_s.local_name->kind == INTERNAL_LOCAL_NAME);
    idx = stmt->u.label_s.local_name->u.int_ln.static_name_index;

    st_parse_tree->static_name[idx].address = st_buf_idx;
}

static void gen_statement(Statement *stmt);

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
    gen_statement(stmt->u.while_s.stmt);
    gencode(BVM_JUMP, start_label);
    set_label(end_label);
}

static void
gen_switch_statement(Statement *stmt)
{
    CaseClause *cc;

    gen_expression(stmt->u.switch_s.value);
    for (cc = stmt->u.switch_s.case_list; cc != NULL; cc = cc->next) {
	gencode(BVM_DUPLICATE);
	cc->label = get_label();
	assert(cc->stmt->kind == CASE_STATEMENT);
	gen_expression(cc->stmt->u.case_s.expr);
	gencode(BVM_EQ);
	gencode(BVM_JUMP_IF_TRUE, cc->label);
    }
    gencode(BVM_POP);
    if (stmt->u.switch_s.default_case) {
	int default_label = get_label();
	stmt->u.switch_s.default_case->label = default_label;
	gencode(BVM_JUMP, default_label);
    }
    gen_statement(stmt->u.switch_s.stmt);
}

static void
gen_statement(Statement *stmt)
{
    switch (stmt->kind) {
    case AUTO_STATEMENT:
	/* nothing to do */
	break;
    case EXTRN_STATEMENT:
	/* nothing to do */
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
	break;
    case BREAK_STATEMENT:
	break;
    case RETURN_STATEMENT:
	break;
    case NULL_STATEMENT:
	break;
    case EXPRESSION_STATEMENT:
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
	set_static_address(def->u.func_def.name);
	gen_statement(def->u.func_def.stmt);
    }
}

int *
bcp_generate_code(ParseTree *parse_tree)
{
    st_opcode_info = BVM_get_opcode_info();

    st_buf = MEM_malloc(sizeof(int) * BVM_MEMORY_SIZE);
    st_parse_tree = parse_tree;

    if (st_labels != NULL) {
	MEM_free(st_labels);
    }
    st_labels = NULL;
    st_label_count = 0;

    gen_functions(parse_tree->def_head);

    return st_buf;
}


