#include <string.h>
#include <assert.h>
#include "BVM.h"
#include "compiler.h"

static char *st_current_function_name;
static LocalName *st_current_function_local;
static int st_static_name_index;
static StaticName *st_static_name;
static int st_string_literal_count;
static StringLiteralDef *st_string_literal_def;

static int st_outer_statement_count;
static int st_outer_statement_alloc_count;
static Statement **st_outer_statement_stack;

static int st_builtin_function_count;

static void fix_statement(Statement *stmt);

static int
add_static_name(char *name, Boolean is_function, Boolean is_vec, int vec_size,
                Boolean defined, IVal *initializer, int line_number)
{
    int i;

    for (i = 0; i < st_static_name_index; i++) {
        if (!strcmp(name, st_static_name[i].name)) {
            break;
        }
    }
    if (i < st_static_name_index) {
        if (defined && st_static_name[i].defined) {
            bcp_compile_error(NAME_ALREADY_DEFINED_ERR,
                              line_number, name);
        }
        if (defined) {
            st_static_name[i].defined = defined;
            st_static_name[i].is_function = is_function;
            st_static_name[i].initializer = initializer;
            if (is_vec) {
                st_static_name[i].is_vec = TRUE;
                st_static_name[i].vec_size = vec_size;
            }
        }
        return i;
    }

    st_static_name
        = MEM_realloc(st_static_name,
                      sizeof(StaticName) * (st_static_name_index + 1));

    st_static_name[i].name = name;
    st_static_name[i].is_function = is_function;
    st_static_name[i].is_vec = is_vec;
    st_static_name[i].vec_size = vec_size;
    st_static_name[i].defined = defined;
    st_static_name[i].initializer = initializer;
    st_static_name_index++;

    return i;
}

static LocalName *
alloc_local_name(LocalNameKind kind,char *name, int line_number)
{
    LocalName *ln;

    ln = bcp_malloc(sizeof(LocalName));
    ln->kind = kind;
    ln->name = name;
    ln->line_number = line_number;
    ln->next = NULL;

    return ln;
}

static LocalName *
add_local_name(LocalNameKind kind, char *name, Boolean is_parameter,
               Boolean is_vec, int vec_size, int line_number)
{
    LocalName *pos;
    LocalName *lp;

    assert(kind == EXTERNAL_LOCAL_NAME || kind == AUTO_LOCAL_NAME);
    assert(!is_parameter || kind == AUTO_LOCAL_NAME);

    if (st_current_function_local != NULL) {
        for (pos = st_current_function_local; pos->next != NULL;
             pos = pos->next) {
            if (!strcmp(name, pos->name)) {
                bcp_compile_error(NAME_ALREADY_DEFINED_ERR,
                                  line_number, name);
            }
        }
        lp = alloc_local_name(kind, name, line_number);
        pos->next = lp;
    } else {
        lp = alloc_local_name(kind, name, line_number);
        st_current_function_local = lp;
    }
    lp->kind = kind;

    switch (kind) {
    case EXTERNAL_LOCAL_NAME:
        lp->u.ext_ln.static_name_index
            = add_static_name(name, FALSE, FALSE, 0, FALSE, NULL, line_number);
        break;
    case INTERNAL_LOCAL_NAME:
        assert(0);
        break;
    case AUTO_LOCAL_NAME:
        lp->u.auto_ln.is_vec = is_vec;
        lp->u.auto_ln.vec_size = vec_size;
        lp->u.auto_ln.is_parameter = is_parameter;
        break;
    default:
        assert(0);
    }

    return lp;
}

static LocalName *
search_local_name(char *name)
{
    LocalName *pos;

    for (pos = st_current_function_local; pos != NULL; pos = pos->next) {
        if (!strcmp(name, pos->name)) {
            return pos;
        }
    }
    return NULL;
}

static LocalName *
add_label(char *name, Boolean defined, int line_number)
{
    LocalName *pos;
    LocalName *tail;
    LocalName *lp;
    char *external_name;

    if (st_current_function_local != NULL) {
        for (pos = st_current_function_local; pos != NULL;
             pos = pos->next) {
            tail = pos;
            if (!strcmp(name, pos->name)) {
                if (defined && pos->u.int_ln.defined) {
                    bcp_compile_error(NAME_ALREADY_DEFINED_ERR,
                                      line_number, name);
                } else {
                    lp = pos;
                    break;
                }
            }
        }
        if (pos == NULL) {
            lp = alloc_local_name(INTERNAL_LOCAL_NAME, name, line_number);
            tail->next = lp;
        }
    } else {
        lp = alloc_local_name(INTERNAL_LOCAL_NAME, name, line_number);
    }

    external_name = bcp_malloc(strlen(st_current_function_name)
                               + 1
                               + strlen(name) + 1);
    sprintf(external_name, "%s:%s", st_current_function_name, name);
    lp->u.int_ln.static_name_index
        = add_static_name(external_name, FALSE, FALSE, 0, defined,
                          NULL, line_number);
    lp->u.int_ln.defined = defined;

    return lp;
}

static int
add_string_literal(StringLiteral sl, Boolean in_code)
{
    int idx = st_string_literal_count;

    st_string_literal_def
        = MEM_realloc(st_string_literal_def,
                      sizeof(StringLiteralDef) * (idx + 1));
    st_string_literal_def[idx].str_literal = sl;
    st_string_literal_def[idx].in_code = in_code;
    st_string_literal_count++;

    return idx;
}

static void
push_outer_statement(Statement *stmt)
{
    assert(st_outer_statement_count <= st_outer_statement_alloc_count);
    if (st_outer_statement_count == st_outer_statement_alloc_count) {
        st_outer_statement_stack
            = MEM_realloc(st_outer_statement_stack,
                          sizeof(Statement*)
                          * (st_outer_statement_alloc_count + 1));
        st_outer_statement_alloc_count++;
    }
    assert(stmt->kind == WHILE_STATEMENT
           || stmt->kind == SWITCH_STATEMENT);
    st_outer_statement_stack[st_outer_statement_count] = stmt;
    st_outer_statement_count++;
}
    
static void
pop_outer_statement(void)
{
    st_outer_statement_count--;
}

static void fix_expression(Expression *expr);

static void
fix_name_expression(Expression *expr, Boolean is_function)
{
    LocalName *ln;

    ln = search_local_name(expr->u.name_e.name);
    if (ln != NULL) {
        expr->u.name_e.local_name = ln;
    } else {
        if (is_function) {
            expr->u.name_e.local_name
                = add_local_name(EXTERNAL_LOCAL_NAME, expr->u.name_e.name,
                                 FALSE, FALSE, 0, expr->line_number);
        } else {
            expr->u.name_e.local_name
                = add_label(expr->u.name_e.name, FALSE, expr->line_number);
        }
    }
    expr->has_lvalue = TRUE;
}

static void
fix_string_literal_expression(Expression *expr)
{
    expr->u.str_e.index
        = add_string_literal(expr->u.str_e.str_literal, TRUE);
}

static void
fix_index_expression(Expression *expr)
{
    fix_expression(expr->u.index_e.vector);
    fix_expression(expr->u.index_e.index);
    expr->has_lvalue = TRUE;
}

static void
fix_args(Argument *head)
{
    Argument *arg;

    for (arg = head; arg != NULL; arg = arg->next) {
        fix_expression(arg->expr);
    }
}

static void
fix_function_call_expression(Expression *expr)
{
    if (expr->u.func_call_e.func->kind == NAME_EXPRESSION) {
        fix_name_expression(expr->u.func_call_e.func, TRUE);
    } else {
        fix_expression(expr->u.func_call_e.func);
    }
    fix_args(expr->u.func_call_e.args);
}

static void
fix_unary_expression(Expression *expr)
{
    fix_expression(expr->u.unary_e.operand);

    if (expr->u.unary_e.operator == INDIRECTION_OPERATOR
        || expr->u.unary_e.operator == PRE_INC_OPERATOR
        || expr->u.unary_e.operator == PRE_DEC_OPERATOR
        || expr->u.unary_e.operator == POST_INC_OPERATOR
        || expr->u.unary_e.operator == POST_DEC_OPERATOR) {
        if (!expr->u.unary_e.operand->has_lvalue) {
            bcp_compile_error(OPERAND_DOES_NOT_HAVE_LVALUE_ERR,
                              expr->line_number);
        }
        expr->u.unary_e.operand->is_lvalue = TRUE;
    }
    if (expr->u.unary_e.operator == INDIRECTION_OPERATOR) {
        expr->has_lvalue = TRUE;
    }
    
}

static void
fix_binary_expression(Expression *expr)
{
    fix_expression(expr->u.binary_e.left);
    fix_expression(expr->u.binary_e.right);

    if (is_assign_operator(expr->u.binary_e.operator)) {
        if (!expr->u.binary_e.left->has_lvalue) {
            bcp_compile_error(OPERAND_DOES_NOT_HAVE_LVALUE_ERR,
                              expr->line_number);
        }
        expr->u.binary_e.left->is_lvalue = TRUE;
    }
}

static void
fix_conditional_expression(Expression *expr)
{
    fix_expression(expr->u.cond_e.cond);
    fix_expression(expr->u.cond_e.expr1);
    fix_expression(expr->u.cond_e.expr2);
}

static void
fix_expression(Expression *expr)
{
    switch (expr->kind) {
    case NAME_EXPRESSION:
        fix_name_expression(expr, FALSE);
        break;
    case INTEGER_LITERAL:
        /* nothing to do */
        break;
    case CHARS_LITERAL:
        /* nothing to do */
        break;
    case STRING_LITERAL:
        fix_string_literal_expression(expr);
        break;
    case INDEX_EXPRESSION:
        fix_index_expression(expr);
        break;
    case FUNCTION_CALL_EXPRESSION:
        fix_function_call_expression(expr);
        break;
    case UNARY_EXPRESSION:
        fix_unary_expression(expr);
        break;
    case BINARY_EXPRESSION:
        fix_binary_expression(expr);
        break;
    case CONDITIONAL_EXPRESSION:
        fix_conditional_expression(expr);
        break;
    default:
        assert(0);
    }
}

static void
fix_auto_statement(Statement *stmt)
{
    NameConstant *pos;

    for (pos = stmt->u.auto_s.name_constant; pos != NULL; pos = pos->next) {
        add_local_name(AUTO_LOCAL_NAME, pos->name, FALSE,
                       pos->is_vec, pos->vec_size, stmt->line_number);
    }
    fix_statement(stmt->u.auto_s.following);
}

static void
fix_extrn_statement(Statement *stmt)
{
    NameItem *pos;

    for (pos = stmt->u.extrn_s.name_list; pos != NULL; pos = pos->next) {
        add_local_name(EXTERNAL_LOCAL_NAME, pos->name, FALSE,
                       FALSE, 0, stmt->line_number);
    }
    fix_statement(stmt->u.extrn_s.following);
}

static void
fix_labeled_statement(Statement *stmt) {
    stmt->u.label_s.local_name
        = add_label(stmt->u.label_s.name, TRUE, stmt->line_number);
    fix_statement(stmt->u.label_s.following);
}

static Statement *
search_nearest_switch(int line_number)
{
    int i;

    for (i = st_outer_statement_count - 1; i >= 0; i++) {
        if (st_outer_statement_stack[i]->kind == SWITCH_STATEMENT) {
            return st_outer_statement_stack[i];
        }
    }
    bcp_compile_error(CASE_WITHOUT_SWITCH_ERR, line_number);

    return NULL; /* make compiler happy. */
}

static void
fix_case_statement(Statement *stmt)
{
    CaseClause *case_p;
    CaseClause *pos;
    Statement *switch_s = search_nearest_switch(stmt->line_number);

    fix_expression(stmt->u.case_s.expr);
    fix_statement(stmt->u.case_s.following);

    case_p = bcp_malloc(sizeof(CaseClause));
    case_p->stmt = stmt;
    case_p->next = NULL;
    stmt->u.case_s.this_case = case_p;

    if (switch_s->u.switch_s.case_list == NULL) {
        switch_s->u.switch_s.case_list = case_p;
    } else {
        for (pos = switch_s->u.switch_s.case_list; pos->next != NULL;
             pos = pos->next)
            ;
        pos->next = case_p;
    }
}

static void
fix_default_statement(Statement *stmt)
{
    Statement *switch_s = search_nearest_switch(stmt->line_number);
    CaseClause *default_p;

    fix_statement(stmt->u.default_s.following);
    default_p = bcp_malloc(sizeof(CaseClause));
    default_p->stmt = stmt;
    default_p->next = NULL;
    stmt->u.default_s.this_case = default_p;

    switch_s->u.switch_s.default_case = default_p;
}

static void
fix_if_statement(Statement *stmt)
{
    fix_expression(stmt->u.if_s.cond);
    fix_statement(stmt->u.if_s.then_clause);
    if (stmt->u.if_s.else_clause != NULL) {
        fix_statement(stmt->u.if_s.else_clause);
    }
}

static void
fix_while_statement(Statement *stmt)
{
    push_outer_statement(stmt);
    fix_expression(stmt->u.while_s.cond);
    fix_statement(stmt->u.while_s.stmt);
    pop_outer_statement();
}

static void
fix_switch_statement(Statement *stmt)
{
    push_outer_statement(stmt);
    fix_expression(stmt->u.switch_s.value);
    fix_statement(stmt->u.switch_s.stmt);
    pop_outer_statement();
}

static void
fix_goto_statement(Statement *stmt)
{
    fix_expression(stmt->u.goto_s.value);
}

static void
fix_break_statement(Statement *stmt)
{
    if (st_outer_statement_count < 1) {
        bcp_compile_error(BREAK_WITHOUT_SWITCH_OR_WHILE_ERR,
                          stmt->line_number);
    }
    stmt->u.break_s.outer
        = st_outer_statement_stack[st_outer_statement_count - 1];
}

static void
fix_return_statement(Statement *stmt)
{
    if (stmt->u.return_s.value != NULL) {
        fix_expression(stmt->u.return_s.value);
    }
}

static void
fix_null_statement(Statement *stmt)
{
    ;
}

static void
fix_expression_statement(Statement *stmt)
{
    fix_expression(stmt->u.expr_s.expr);
}

static void
fix_compound_statement(Statement *stmt)
{
    Statement *pos;

    for (pos = stmt->u.compound_s.stmt; pos != NULL; pos = pos->next) {
        fix_statement(pos);
    }
}

static void
fix_statement(Statement *stmt)
{
    switch (stmt->kind) {
    case AUTO_STATEMENT:
        fix_auto_statement(stmt);
        break;
    case EXTRN_STATEMENT:
        fix_extrn_statement(stmt);
        break;
    case LABELED_STATEMENT:
        fix_labeled_statement(stmt);
        break;
    case CASE_STATEMENT:
        fix_case_statement(stmt);
        break;
    case DEFAULT_STATEMENT:
        fix_default_statement(stmt);
        break;
    case COMPOUND_STATEMENT:
        fix_compound_statement(stmt);
        break;
    case IF_STATEMENT:
        fix_if_statement(stmt);
        break;
    case WHILE_STATEMENT:
        fix_while_statement(stmt);
        break;
    case SWITCH_STATEMENT:
        fix_switch_statement(stmt);
        break;
    case GOTO_STATEMENT:
        fix_goto_statement(stmt);
        break;
    case BREAK_STATEMENT:
        fix_break_statement(stmt);
        break;
    case RETURN_STATEMENT:
        fix_return_statement(stmt);
        break;
    case NULL_STATEMENT:
        fix_null_statement(stmt);
        break;
    case EXPRESSION_STATEMENT:
        fix_expression_statement(stmt);
        break;
    default:
        assert(0);
    }
}

static void
add_parameters(NameItem *params, int line_number)
{
    NameItem *pos;

    for (pos = params; pos != NULL; pos = pos->next) {
        add_local_name(AUTO_LOCAL_NAME, pos->name, TRUE,
                       FALSE, 0, line_number);
    }
}

static int
set_auto_offset(void)
{
    LocalName *pos;
    int offset = 0;
    int local_variable_size;

    for (pos = st_current_function_local; pos != NULL; pos = pos->next) {
        if (pos->kind != AUTO_LOCAL_NAME || pos->u.auto_ln.is_parameter) {
            continue;
        }
        pos->u.auto_ln.offset = offset;
        if (pos->u.auto_ln.is_vec) {
            offset += pos->u.auto_ln.vec_size;
        } else {
            offset++;
        }
    }
    local_variable_size = offset;
    offset += BVM_RETURN_INFO_SIZE + 1;
    for (pos = st_current_function_local; pos != NULL; pos = pos->next) {
        if (pos->kind != AUTO_LOCAL_NAME || !pos->u.auto_ln.is_parameter) {
            continue;
        }
        pos->u.auto_ln.offset = offset;
        offset++;
    }

    return local_variable_size;
}

static void
check_local_variable(void)
{
    LocalName *ln;

    for (ln = st_current_function_local; ln != NULL; ln = ln->next) {
        if (ln->kind == INTERNAL_LOCAL_NAME
            && !ln->u.int_ln.defined) {
            bcp_compile_error(NAME_NOT_DEFINED_ERR, ln->line_number,
                              ln->name);
        }
    }
}

static void
fix_function_definition(Definition *def)
{
    int local_variable_size;
    st_current_function_local = NULL;
    st_current_function_name = def->u.func_def.name;

    add_parameters(def->u.func_def.params, def->line_number);
    fix_statement(def->u.func_def.stmt);
    check_local_variable();
    local_variable_size = set_auto_offset();

    def->u.func_def.local_names = st_current_function_local;
    def->u.func_def.local_variable_size = local_variable_size;
    st_current_function_name = NULL;

    add_static_name(def->u.func_def.name, TRUE, FALSE, 0, TRUE,
                    NULL, def->line_number);
}

static int
get_ext_vec_size(Definition *def)
{
    int ival_count = 0;
    IVal *ival_p;

    if (!def->u.decl_def.has_vec_size
        && def->u.decl_def.ival_list == NULL) {
        bcp_compile_error(VECTOR_SIZE_IS_NOT_DEFINED_ERR, def->line_number,
                          def->u.decl_def.name);
    }
    for (ival_p = def->u.decl_def.ival_list; ival_p != NULL;
         ival_p = ival_p->next) {
        ival_count++;
    }
    if (!def->u.decl_def.has_vec_size) {
        return ival_count;
    } else {
        if (def->u.decl_def.has_vec_size > ival_count) {
            return def->u.decl_def.vec_size;
        } else {
            return ival_count;
        }
    }
}

static int
search_static_name(char *name, int max_idx, int line_number)
{
    int i;

    for (i = 0; i < max_idx; i++) {
        if (!strcmp(name, st_static_name[i].name)) {
            return i;
        }
    }
    bcp_compile_error(NAME_NOT_FOUND_ERR, line_number, name);
    return 0; /* make compiler happy. */
}

static void
fix_declaration(Definition *def)
{
    int vec_size;
    IVal *pos;
    int name_idx;

    vec_size = get_ext_vec_size(def);

    name_idx = add_static_name(def->u.decl_def.name,
                               FALSE, def->u.decl_def.is_vec, vec_size,
                               TRUE, def->u.decl_def.ival_list,
                               def->line_number);
    for (pos = def->u.decl_def.ival_list; pos != NULL; pos = pos->next) {
        if (pos->kind == CONSTANT_IVAL
            && pos->u.c.constant->kind == STRING_CONSTANT) {
            pos->u.c.string_literal_index
                = add_string_literal(pos->u.c.constant->u.str_literal, FALSE);
        } else if (pos->kind == NAME_IVAL) {
            pos->u.n.static_name_index
                = search_static_name(pos->u.n.name, name_idx, def->line_number);
        }
    }
}

static void
add_builtin_functions(void)
{
    int i;
    char **builtin_functions
        = BVM_get_builtin_functions(&st_builtin_function_count);

    for (i = 0; i < st_builtin_function_count; i++) {
        add_static_name(builtin_functions[i],
                        TRUE, FALSE, 0, TRUE, NULL, 0);
    }
}

ParseTree *
bcp_fix_tree(Definition *def_head)
{
    ParseTree *ret;
    Definition *def;

    st_static_name_index = 0;
    if (st_static_name != NULL) {
        MEM_free(st_static_name);
    }
    st_static_name = NULL;

    st_outer_statement_count = 0;
    st_outer_statement_alloc_count = 0;
    if (st_outer_statement_stack != NULL) {
        MEM_free(st_outer_statement_stack);
    }
    st_outer_statement_stack = NULL;

    add_builtin_functions();

    for (def = def_head; def != NULL; def = def->next) {
        if (def->kind == FUNCTION_DEFINITION) {
            fix_function_definition(def);
        } else {
            assert(def->kind == DECLARATION_DEFINITION);
            fix_declaration(def);
        }
    }
    ret= bcp_malloc(sizeof(ParseTree));
    ret->def_head = def_head;
    ret->static_name_count = st_static_name_index;
    ret->static_name = st_static_name;
    ret->string_literal_count = st_string_literal_count;
    ret->string_literal = st_string_literal_def;

    return ret;
}
