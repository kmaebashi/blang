#include <stdio.h>
#include <assert.h>
#include "compiler.h"

#define INDENT_WIDTH (2)

static char *st_unary_operator_str[] = {
    "INDIRECTION",
    "ADDRESS",
    "MINUS",
    "LOGICAL_NOT",
    "PRE_INC",
    "PRE_DEC",
    "POST_INC",
    "POST_DEC",
    "BIT_NOT",
};

static char *st_binary_operator_str[] = {
    "MUL",
    "DIV",
    "MOD",
    "ADD",
    "SUB",
    "LEFT_SHIFT",
    "RIGHT_SHIFT",
    "LT",
    "LE",
    "GT",
    "GE",
    "EQ",
    "NE",
    "BIT_AND",
    "BIT_XOR",
    "BIT_OR",
    "ASSIGN",
    "ADD_ASSIGN",
    "SUB_ASSIGN",
    "MUL_ASSIGN",
    "DIV_ASSIGN",
    "MOD_ASSIGN",
    "LEFT_SHIFT_ASSIGN",
    "RIGHT_SHIFT_ASSIGN",
    "LT_ASSIGN",
    "LE_ASSIGN",
    "GT_ASSIGN",
    "GE_ASSIGN",
    "EQ_ASSIGN",
    "NE_ASSIGN",
    "BIT_AND_ASSIGN",
    "BIT_XOR_ASSIGN",
    "BIT_OR_ASSIGN",
};

static void
out_indent(FILE *fp, int level) {
    int i;

    for (i = 0; i < level * INDENT_WIDTH; i++) {
	putc(' ', fp);
    }
}

static void dump_expression(FILE *fp, Expression *expr, int level);

static void
dump_name_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "name..%s\n", expr->u.name_e.name);
}

static void
dump_integer_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "int_literal..%d\n", expr->u.int_e.int_value);
}

static void
dump_chars_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "chars..%08x\n", expr->u.chars_e.chars);
}

static void
dump_string_literal_expression(FILE *fp, Expression *expr, int level)
{
    int i;

    out_indent(fp, level);
    fprintf(fp, "string..\"");

    for (i = 0; i < expr->u.str_e.str_literal.length; i++) {
	fprintf(fp, "%c", expr->u.str_e.str_literal.str[i]);
    }
    fprintf(fp, "\"\n");
}

static void
dump_index_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "index\n");
    out_indent(fp, level);
    fprintf(fp, "vector:\n");
    dump_expression(fp, expr->u.index_e.vector, level + 1);
    out_indent(fp, level);
    fprintf(fp, "index:\n");
    dump_expression(fp, expr->u.index_e.index, level + 1);
}

static void
dump_args(FILE *fp, Argument *head, int level)
{
    Argument *arg;
    int count = 1;

    for (arg = head; arg != NULL; arg = arg->next) {
	out_indent(fp, level);
	fprintf(fp, "arg[%d]:\n", count);
	count++;
	dump_expression(fp, arg->expr, level + 1);
    }
}

static void
dump_function_call_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "function call\n");
    out_indent(fp, level);
    fprintf(fp, "func:\n");
    dump_expression(fp, expr->u.func_call_e.func, level + 1);
    out_indent(fp, level);
    fprintf(fp, "args:\n");
    dump_args(fp, expr->u.func_call_e.args, level + 1);
}

static void
dump_unary_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "unary %s\n", st_unary_operator_str[expr->u.unary_e.operator]);
    out_indent(fp, level);
    fprintf(fp, "operand:\n");
    dump_expression(fp, expr->u.unary_e.operand, level + 1);
}

static void
dump_binary_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "binary %s\n",
	    st_binary_operator_str[expr->u.binary_e.operator]);
    out_indent(fp, level);
    fprintf(fp, "left:\n");
    dump_expression(fp, expr->u.binary_e.left, level + 1);
    out_indent(fp, level);
    fprintf(fp, "right:\n");
    dump_expression(fp, expr->u.binary_e.right, level + 1);
}

static void
dump_conditional_expression(FILE *fp, Expression *expr, int level)
{
    out_indent(fp, level);
    fprintf(fp, "conditional\n");
    out_indent(fp, level);
    fprintf(fp, "cond:\n");
    dump_expression(fp,expr->u.cond_e.cond, level + 1);
    out_indent(fp, level);
    fprintf(fp, "expr1:\n");
    dump_expression(fp,expr->u.cond_e.expr1, level + 1);
    out_indent(fp, level);
    fprintf(fp, "expr2:\n");
    dump_expression(fp,expr->u.cond_e.expr2, level + 1);
}

static void
dump_expression(FILE *fp, Expression *expr, int level)
{
    switch (expr->kind) {
    case NAME_EXPRESSION:
	dump_name_expression(fp, expr, level);
	break;
    case INTEGER_LITERAL:
	dump_integer_expression(fp, expr, level);
	break;
    case CHARS_LITERAL:
	dump_chars_expression(fp, expr, level);
	break;
    case STRING_LITERAL:
	dump_string_literal_expression(fp, expr, level);
	break;
    case INDEX_EXPRESSION:
	dump_index_expression(fp, expr, level);
	break;
    case FUNCTION_CALL_EXPRESSION:
	dump_function_call_expression(fp, expr, level);
	break;
    case UNARY_EXPRESSION:
	dump_unary_expression(fp, expr, level);
	break;
    case BINARY_EXPRESSION:
	dump_binary_expression(fp, expr, level);
	break;
    case CONDITIONAL_EXPRESSION:
	dump_conditional_expression(fp, expr, level);
	break;
    default:
	assert(0);
    }

}

static void
dump_names(FILE *fp, NameItem *head)
{
    NameItem *pos;

    for (pos = head; pos != NULL; pos = pos->next) {
	fprintf(fp, "%s", pos->name);
	if (pos->next != NULL) {
	    fprintf(fp, ", ");
	}
    }
}

static void
dump_constant(FILE *fp, Constant *constant)
{
    switch (constant->kind) {
    case INT_CONSTANT:
	fprintf(fp, "%d", constant->u.int_value);
	break;
    case CHARS_CONSTANT:
	fprintf(fp, "%08x", constant->u.chars);
	break;
    case STRING_CONSTANT:
	fprintf(fp, "%s", constant->u.str_literal.str);
	break;
    default:
	assert(0);
    }
}

static void dump_statement(FILE *fp, Statement *head, int level);

static void
dump_name_constants(FILE *fp, NameConstant *head)
{
    NameConstant *nc;

    for (nc = head; nc != NULL; nc = nc->next) {
	fprintf(fp, "%s", nc->name);

	if (nc->is_vec) {
	    fprintf(fp, "[%d]", nc->vec_size);
	}
	if (nc->next != NULL) {
	    fprintf(fp, ", ");
	}
    }
}

static void
dump_auto_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "auto ");
    dump_name_constants(fp, stmt->u.auto_s.name_constant);
    fprintf(fp, "\n");

    dump_statement(fp, stmt->u.auto_s.following, level);
}

static void
dump_extrn_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "extrn ");
    dump_names(fp, stmt->u.extrn_s.name_list);
    fprintf(fp, "\n");

    dump_statement(fp, stmt->u.extrn_s.following, level);
}

static void
dump_labeled_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "label(%s)", stmt->u.label_s.name);
    fprintf(fp, "\n");

    dump_statement(fp, stmt->u.label_s.following, level);
}

static void
dump_case_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "case ");
    dump_constant(fp, stmt->u.case_s.constant);
    fprintf(fp, ":\n");

    dump_statement(fp, stmt->u.case_s.following, level);
}

static void
dump_default_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "default:\n");

    dump_statement(fp, stmt->u.default_s.following, level);
}

static void
dump_if_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "if\n");
    out_indent(fp, level);
    fprintf(fp, "cond:\n");
    dump_expression(fp, stmt->u.if_s.cond, level + 1);
    out_indent(fp, level);
    fprintf(fp, "then:\n");
    dump_statement(fp, stmt->u.if_s.then_clause, level + 1);
    if (stmt->u.if_s.else_clause != NULL) {
	out_indent(fp, level);
	fprintf(fp, "else:\n");
	dump_statement(fp, stmt->u.if_s.else_clause, level + 1);
    }
}

static void
dump_while_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "while\n");
    out_indent(fp, level);
    fprintf(fp, "cond:\n");
    dump_expression(fp, stmt->u.while_s.cond, level + 1);
    out_indent(fp, level);
    fprintf(fp, "statement:\n");
    dump_statement(fp, stmt->u.while_s.stmt, level + 1);
}

static void
dump_switch_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "switch\n");
    out_indent(fp, level);
    fprintf(fp, "value:\n");
    dump_expression(fp, stmt->u.switch_s.value, level + 1);
    out_indent(fp, level);
    fprintf(fp, "statement:\n");
    dump_statement(fp, stmt->u.switch_s.stmt, level + 1);
}

static void
dump_goto_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "goto\n");
    out_indent(fp, level);
    fprintf(fp, "value:\n");
    dump_expression(fp, stmt->u.goto_s.value, level + 1);
}

static void
dump_break_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "break\n");
}

static void
dump_return_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "return\n");
    if (stmt->u.return_s.value != NULL) {
	out_indent(fp, level);
	fprintf(fp, "value:\n");
	dump_expression(fp, stmt->u.return_s.value, level + 1);
    }
}

static void
dump_null_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "null_statement\n");
}

static void
dump_expression_statement(FILE *fp, Statement *stmt, int level)
{
    out_indent(fp, level);
    fprintf(fp, "expression_statement\n");
    out_indent(fp, level);
    fprintf(fp, "expr:\n");
    dump_expression(fp, stmt->u.expr_s.expr, level + 1);
}

static void
dump_compound_statement(FILE *fp, Statement *stmt, int level)
{
    Statement *pos;

    out_indent(fp, level);
    fprintf(fp, "{\n");

    for (pos = stmt->u.compound_s.stmt; pos != NULL; pos = pos->next) {
	dump_statement(fp, pos, level + 1);
    }

    out_indent(fp, level);
    fprintf(fp, "}\n");
}

static void
dump_statement(FILE *fp, Statement *stmt, int level)
{
    switch (stmt->kind) {
    case AUTO_STATEMENT:
	dump_auto_statement(fp, stmt, level);
	break;
    case EXTRN_STATEMENT:
	dump_extrn_statement(fp, stmt, level);
	break;
    case LABELED_STATEMENT:
	dump_labeled_statement(fp, stmt, level);
	break;
    case CASE_STATEMENT:
	dump_case_statement(fp, stmt, level);
	break;
    case DEFAULT_STATEMENT:
	dump_default_statement(fp, stmt, level);
	break;
    case COMPOUND_STATEMENT:
	dump_compound_statement(fp, stmt, level);
	break;
    case IF_STATEMENT:
	dump_if_statement(fp, stmt, level);
	break;
    case WHILE_STATEMENT:
	dump_while_statement(fp, stmt, level);
	break;
    case SWITCH_STATEMENT:
	dump_switch_statement(fp, stmt, level);
	break;
    case GOTO_STATEMENT:
	dump_goto_statement(fp, stmt, level);
	break;
    case BREAK_STATEMENT:
	dump_break_statement(fp, stmt, level);
	break;
    case RETURN_STATEMENT:
	dump_return_statement(fp, stmt, level);
	break;
    case NULL_STATEMENT:
	dump_null_statement(fp, stmt, level);
	break;
    case EXPRESSION_STATEMENT:
	dump_expression_statement(fp, stmt, level);
	break;
    default:
	assert(0);
    }
}

static void
dump_function_definition(FILE *fp, Definition *def, int level)
{
    out_indent(fp, level);
    fprintf(fp, "function definition %s(", def->u.func_def.name);
    dump_names(fp, def->u.func_def.params);
    fprintf(fp, ")\n");

    dump_statement(fp, def->u.func_def.stmt, level);
}

static void
dump_declaration(FILE *fp, Definition *def, int level)
{
    IVal *ival_p;

    out_indent(fp, level);
    fprintf(fp, "declaration %s", def->u.decl_def.name);

    if (def->u.decl_def.is_vec) {
	fprintf(fp, "[");
	if (def->u.decl_def.has_vec_size) {
	    fprintf(fp, "%d", def->u.decl_def.vec_size);
	}
	fprintf(fp, "] ");
    }
    for (ival_p = def->u.decl_def.ival_list; ival_p != NULL;
	 ival_p = ival_p->next) {
	if (ival_p->kind == CONSTANT_IVAL) {
	    dump_constant(fp, ival_p->u.constant);
	} else {
	    assert(ival_p->kind == NAME_IVAL);
	    fprintf(fp, "%s", ival_p->u.name);
	}
	if (ival_p->next != NULL) {
	    fprintf(fp, ", ");
	}
    }
    fprintf(fp, "\n");
}

void
bcp_dump_tree(FILE *fp, Definition* def_head)
{
    Definition *def;

    for (def = def_head; def != NULL; def = def->next) {
	if (def->kind == FUNCTION_DEFINITION) {
	    dump_function_definition(fp, def, 0);
	} else {
	    assert(def->kind == DECLARATION_DEFINITION);
	    dump_declaration(fp, def, 0);
	}
    }
}
