#include <string.h>
#include <assert.h>
#include "compiler.h"

#define LOOK_AHEAD_TOKEN_COUNT (2)
static Token st_look_ahead_token_stack[LOOK_AHEAD_TOKEN_COUNT];
static int st_look_ahead_token_count = 0;

static void
unget_token(Token token)
{
    assert(st_look_ahead_token_count < LOOK_AHEAD_TOKEN_COUNT);

    st_look_ahead_token_stack[st_look_ahead_token_count] = token;
    st_look_ahead_token_count++;
}

static Token
get_token(void)
{
    Token ret;

    if (st_look_ahead_token_count > 0) {
        ret = st_look_ahead_token_stack[st_look_ahead_token_count - 1];
        st_look_ahead_token_count--;
    } else {
        ret = bcp_get_token();
    }

    return ret;
}

static void check_token(Token token, TokenKind kind)
{
    if (token.kind != kind) {
        bcp_compile_error(UNEXPECTED_TOKEN_ERR,
                          token.line_number,
                          bcp_token_str[token.kind],
                          bcp_token_str[kind]);
    }
}

static void check_token2(Token token,
                         TokenKind expected1, TokenKind expected2)
{
    if (token.kind != expected1 && token.kind != expected2) {
        bcp_compile_error(UNEXPECTED_TOKEN2_ERR,
                          token.line_number,
                          bcp_token_str[token.kind],
                          bcp_token_str[expected1],
                          bcp_token_str[expected2]);
    }
}


static NameItem *parse_parameters(void)
{
    Token name_token;
    Token comma_token;
    NameItem *param;
    NameItem *head = NULL;
    NameItem *tail = NULL;

    name_token = get_token();
    if (name_token.kind == RP_TOKEN) {
        return NULL;
    }
    for (;;) {
        check_token(name_token, NAME_TOKEN);
        param = bcp_malloc(sizeof(NameItem));
        param->name = name_token.u.name;
        param->next = NULL;
        if (head == NULL) {
            head = param;
        } else {
            tail->next = param;
        }
        tail = param;
        comma_token = get_token();
        if (comma_token.kind == RP_TOKEN) {
            break;
        }
        check_token(comma_token, COMMA_TOKEN);
        name_token = get_token();
    }

    return head;
}

static Statement *alloc_statement(StatementKind kind, int line_number)
{
    Statement *ret;

    ret = bcp_malloc(sizeof(Statement));
    ret->kind = kind;
    ret->line_number = line_number;
    ret->next = NULL;

    return ret;
}

static Constant *
parse_constant(void)
{
    Constant *ret;
    Token token;

    ret = bcp_malloc(sizeof(Constant));
    token = get_token();
    ret->line_number = token.line_number;
    if (token.kind == INT_LITERAL_TOKEN) {
        ret->kind = INT_CONSTANT;
        ret->u.int_value = token.u.int_value;
    } else if (token.kind == STRING_LITERAL_TOKEN) {
        ret->kind = STRING_CONSTANT;
        ret->u.str_literal = token.u.str_literal;
    } else if (token.kind == CHARS_TOKEN) {
        ret->kind = CHARS_CONSTANT;
        ret->u.chars = token.u.chars;
    } else {
        bcp_compile_error(UNEXPECTED_TOKEN0_ERR, token.line_number,
                          bcp_token_str[token.kind]);
    }

    return ret;
}


static Expression *parse_expression(void);

static Expression *
alloc_expression(ExpressionKind kind, int line_number)
{
    Expression *ret = bcp_malloc(sizeof(Expression));
    ret->kind = kind;
    ret->line_number = line_number;
    ret->has_lvalue = FALSE;
    
    return ret;
}

static Expression *
constant_to_expression(Constant *c)
{
    Expression *expr;

    switch (c->kind) {
    case INT_CONSTANT:
        expr = alloc_expression(INTEGER_LITERAL, c->line_number);
        expr->u.int_e.int_value = c->u.int_value;
        break;
    case CHARS_CONSTANT:
        expr = alloc_expression(CHARS_LITERAL, c->line_number);
        expr->u.chars_e.chars = c->u.chars;
        break;
    case STRING_CONSTANT:
        expr = alloc_expression(STRING_LITERAL, c->line_number);
        expr->u.str_e.str_literal = c->u.str_literal;
        break;
    }

    return expr;
}

static Expression *
parse_primary_expression()
{
    Token first_token = get_token();
    Token end_token;
    Expression *expr;

    if (first_token.kind == NAME_TOKEN) {
        expr = alloc_expression(NAME_EXPRESSION, first_token.line_number);
        expr->u.name_e.name = first_token.u.name;
    } else if (first_token.kind == INT_LITERAL_TOKEN) {
        expr = alloc_expression(INTEGER_LITERAL, first_token.line_number);
        expr->u.int_e.int_value = first_token.u.int_value;
    } else if (first_token.kind == CHARS_TOKEN) {
        expr = alloc_expression(CHARS_LITERAL, first_token.line_number);
        expr->u.chars_e.chars = first_token.u.chars;
    } else if (first_token.kind == STRING_LITERAL_TOKEN) {
        expr = alloc_expression(STRING_LITERAL, first_token.line_number);
        expr->u.str_e.str_literal = first_token.u.str_literal;
    } else if (first_token.kind == LP_TOKEN) {
        expr = parse_expression();
        end_token = get_token();
        check_token(end_token, RP_TOKEN);
    } else {
        bcp_compile_error(UNEXPECTED_TOKEN0_ERR, first_token.line_number,
                          bcp_token_str[first_token.kind]);
    }

    return expr;
}

static Argument *
parse_arguments()
{
    Token token;
    Argument *head = NULL;
    Argument *tail = NULL;
    Argument *arg;
    Expression *expr;

    token = get_token();
    if (token.kind == RP_TOKEN) {
        return NULL;
    }
    unget_token(token);
    for (;;) {
        expr = parse_expression();
        arg = bcp_malloc(sizeof(Argument));
        arg->expr = expr;
        arg->next = NULL;
        if (head == NULL) {
            head = arg;
        } else {
            tail->next = arg;
        }
        tail = arg;

        token = get_token();
        if (token.kind == RP_TOKEN) {
            break;
        } else {
            check_token(token, COMMA_TOKEN);
        }
    }
    return head;
}

static Expression *
parse_postfix_expression()
{
    Token token;
    Expression *operand;
    Expression *expr;

    operand = parse_primary_expression();

    for (;;) {
        token = get_token();
        if (token.kind == LP_TOKEN) {
            expr = alloc_expression(FUNCTION_CALL_EXPRESSION,
                                    token.line_number);
            expr->u.func_call_e.func = operand;
            expr->u.func_call_e.args = parse_arguments();
            operand = expr;
        } else if (token.kind == LB_TOKEN) {
            expr = alloc_expression(INDEX_EXPRESSION, token.line_number);
            expr->u.index_e.vector = operand;
            expr->u.index_e.index = parse_expression();
            token = get_token();
            check_token(token, RB_TOKEN);
            operand = expr;
        } else if (token.kind == INC_TOKEN) {
            expr = alloc_expression(UNARY_EXPRESSION, token.line_number);
            expr->u.unary_e.operator = POST_INC_OPERATOR;
            expr->u.unary_e.operand = operand;
            operand = expr;
        } else if (token.kind == DEC_TOKEN) {
            expr = alloc_expression(UNARY_EXPRESSION, token.line_number);
            expr->u.unary_e.operator = POST_DEC_OPERATOR;
            expr->u.unary_e.operand = operand;
            operand = expr;
        } else {
            unget_token(token);
            return operand;
        }
    }

    return expr;
}

static Expression *
parse_unary_expression()
{
    Token token;
    Expression *operand;
    Expression *expr;
    UnaryOperator op;
    
    token = get_token();
    if (token.kind == ASTERISK_TOKEN) {
        op = INDIRECTION_OPERATOR;
    } else if (token.kind == AMPERSAND_TOKEN) {
        op = ADDRESS_OPERATOR;
    } else if (token.kind == MINUS_TOKEN) {
        op = MINUS_OPERATOR;
    } else if (token.kind == EXCLAMATION_TOKEN) {
        op = LOGICAL_NOT_OPERATOR;
    } else if (token.kind == INC_TOKEN) {
        op = PRE_INC_OPERATOR;
    } else if (token.kind == DEC_TOKEN) {
        op = PRE_DEC_OPERATOR;
    } else if (token.kind == BIT_NOT_TOKEN) {
        op = BIT_NOT_OPERATOR;
    } else {
        unget_token(token);
        return parse_postfix_expression();
    }
    operand = parse_unary_expression();
    expr = alloc_expression(UNARY_EXPRESSION, token.line_number);
    expr->u.unary_e.operator = op;
    expr->u.unary_e.operand = operand;

    return expr;
}

static Expression *
fold_multiplicative_expression(BinaryOperator op,
                               Expression *left, Expression *right,
                               int line_number)
{
    if (left->kind == INTEGER_LITERAL && right->kind == INTEGER_LITERAL) {
        Expression *expr;

        expr = alloc_expression(INTEGER_LITERAL, line_number);
        if (op == MUL_OPERATOR) {
            expr->u.int_e.int_value
                = left->u.int_e.int_value * right->u.int_e.int_value;
        } else if (op == DIV_OPERATOR) {
            expr->u.int_e.int_value
                = left->u.int_e.int_value / right->u.int_e.int_value;
        } else {
            assert(op == MOD_OPERATOR);
            expr->u.int_e.int_value
                = left->u.int_e.int_value % right->u.int_e.int_value;
        }
        return expr;
    }
    return NULL;
}

static Expression *
parse_multiplicative_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_unary_expression();

    for (;;) {
        token = get_token();
        if (token.kind == ASTERISK_TOKEN) {
            op = MUL_OPERATOR;
        } else if (token.kind == SLASH_TOKEN) {
            op = DIV_OPERATOR;
        } else if (token.kind == PERCENT_TOKEN) {
            op = MOD_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_unary_expression();
        expr = fold_multiplicative_expression(op, left, right,
                                              token.line_number);
        if (expr == NULL) {
            expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
            expr->u.binary_e.operator = op;
            expr->u.binary_e.left = left;
            expr->u.binary_e.right = right;
        }
        left = expr;
    }

    return left;
}

static Expression *
fold_additive_expression(BinaryOperator op,
                         Expression *left, Expression *right,
                         int line_number)
{
    if (left->kind == INTEGER_LITERAL && right->kind == INTEGER_LITERAL) {
        Expression *expr;

        expr = alloc_expression(INTEGER_LITERAL, line_number);
        if (op == ADD_OPERATOR) {
            expr->u.int_e.int_value
                = left->u.int_e.int_value + right->u.int_e.int_value;
        } else {
            assert(op == SUB_OPERATOR);
            expr->u.int_e.int_value
                = left->u.int_e.int_value - right->u.int_e.int_value;
        }

        return expr;
    }
    return NULL;
}

static Expression *
parse_additive_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_multiplicative_expression();

    for (;;) {
        token = get_token();
        if (token.kind == PLUS_TOKEN) {
            op = ADD_OPERATOR;
        } else if (token.kind == MINUS_TOKEN) {
            op = SUB_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_multiplicative_expression();

        expr = fold_additive_expression(op, left, right, token.line_number);
        if (expr == NULL) {
            expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
            expr->u.binary_e.operator = op;
            expr->u.binary_e.left = left;
            expr->u.binary_e.right = right;
        }
        left = expr;
    }

    return left;
}

static Expression *
parse_shift_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_additive_expression();

    for (;;) {
        token = get_token();
        if (token.kind == LEFT_BIT_SHIFT_TOKEN) {
            op = LEFT_SHIFT_OPERATOR;
        } else if (token.kind == RIGHT_BIT_SHIFT_TOKEN) {
            op = RIGHT_SHIFT_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_additive_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = op;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}


static Expression *
parse_relational_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_shift_expression();

    for (;;) {
        token = get_token();
        if (token.kind == LT_TOKEN) {
            op = LT_OPERATOR;
        } else if (token.kind == LE_TOKEN) {
            op = LE_OPERATOR;
        } else if (token.kind == GT_TOKEN) {
            op = GT_OPERATOR;
        } else if (token.kind == GE_TOKEN) {
            op = GE_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_shift_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = op;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}


static Expression *
parse_equality_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_relational_expression();

    for (;;) {
        token = get_token();
        if (token.kind == EQ_TOKEN) {
            op = EQ_OPERATOR;
        } else if (token.kind == NE_TOKEN) {
            op = NE_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_relational_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = op;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}

static Expression *
parse_and_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;

    left = parse_equality_expression();

    for (;;) {
        token = get_token();
        if (token.kind != AMPERSAND_TOKEN) {
            unget_token(token);
            return left;
        }

        right = parse_equality_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = BIT_AND_OPERATOR;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}

static Expression *
parse_xor_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;

    left = parse_and_expression();

    for (;;) {
        token = get_token();
        if (token.kind != BIT_XOR_TOKEN) {
            unget_token(token);
            return left;
        }

        right = parse_and_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = BIT_XOR_OPERATOR;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}

static Expression *
parse_or_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;

    left = parse_xor_expression();

    for (;;) {
        token = get_token();
        if (token.kind != BIT_OR_TOKEN) {
            unget_token(token);
            return left;
        }

        right = parse_xor_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = BIT_OR_OPERATOR;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}

static Expression *
parse_conditional_expression()
{
    Token question_token;
    Token colon_token;
    Expression *left;
    Expression *center;
    Expression *right;
    Expression *expr;

    left = parse_or_expression();

    question_token = get_token();
    if (question_token.kind != QUESTION_TOKEN) {
        unget_token(question_token);
        return left;
    }
    center = parse_conditional_expression();

    colon_token = get_token();
    if (colon_token.kind != COLON_TOKEN) {
        bcp_compile_error(UNEXPECTED_TOKEN_ERR, colon_token.line_number,
                          bcp_token_str[colon_token.kind],
                          bcp_token_str[COLON_TOKEN]);
    }
    right = parse_conditional_expression();
    expr = alloc_expression(CONDITIONAL_EXPRESSION,
                            question_token.line_number);
    expr->u.cond_e.cond = left;
    expr->u.cond_e.expr1 = center;
    expr->u.cond_e.expr2 = right;

    return expr;
}

static Expression *
parse_assignment_expression()
{
    Token token;
    Expression *left;
    Expression *right;
    Expression *expr;
    BinaryOperator op;

    left = parse_conditional_expression();

    for (;;) {
        token = get_token();
        if (token.kind == ASSIGN_TOKEN) {
            op = ASSIGN_OPERATOR;
        } else if (token.kind == ADD_ASSIGN_TOKEN) {
            op = ADD_ASSIGN_OPERATOR;
        } else if (token.kind == SUB_ASSIGN_TOKEN) {
            op = SUB_ASSIGN_OPERATOR;
        } else if (token.kind == MUL_ASSIGN_TOKEN) {
            op = MUL_ASSIGN_OPERATOR;
        } else if (token.kind == DIV_ASSIGN_TOKEN) {
            op = DIV_ASSIGN_OPERATOR;
        } else if (token.kind == MOD_ASSIGN_TOKEN) {
            op = MOD_ASSIGN_OPERATOR;
        } else if (token.kind == LEFT_SHIFT_ASSIGN_TOKEN) {
            op = LEFT_SHIFT_ASSIGN_OPERATOR;
        } else if (token.kind == RIGHT_SHIFT_ASSIGN_TOKEN) {
            op = RIGHT_SHIFT_ASSIGN_OPERATOR;
        } else if (token.kind == LT_ASSIGN_TOKEN) {
            op = LT_ASSIGN_OPERATOR;
        } else if (token.kind == LE_ASSIGN_TOKEN) {
            op = LE_ASSIGN_OPERATOR;
        } else if (token.kind == GT_ASSIGN_TOKEN) {
            op = GT_ASSIGN_OPERATOR;
        } else if (token.kind == GE_ASSIGN_TOKEN) {
            op = GE_ASSIGN_OPERATOR;
        } else if (token.kind == EQ_ASSIGN_TOKEN) {
            op = EQ_ASSIGN_OPERATOR;
        } else if (token.kind == NE_ASSIGN_TOKEN) {
            op = NE_ASSIGN_OPERATOR;
        } else if (token.kind == BIT_AND_ASSIGN_TOKEN) {
            op = BIT_AND_ASSIGN_OPERATOR;
        } else if (token.kind == BIT_OR_ASSIGN_TOKEN) {
            op = BIT_OR_ASSIGN_OPERATOR;
        } else if (token.kind == BIT_XOR_ASSIGN_TOKEN) {
            op = BIT_XOR_ASSIGN_OPERATOR;
        } else {
            unget_token(token);
            return left;
        }
        right = parse_conditional_expression();
        expr = alloc_expression(BINARY_EXPRESSION, token.line_number);
        expr->u.binary_e.operator = op;
        expr->u.binary_e.left = left;
        expr->u.binary_e.right = right;
        left = expr;
    }

    return left;
}

static Expression *
parse_expression(void)
{
    return parse_assignment_expression();
}

static Statement *parse_statement(void);

static Statement *
parse_auto_statement(int line_number)
{
    Statement *ret;
    Token name_token;
    Token lb_token;
    Token rb_token;
    Token next_token;
    NameConstant *head = NULL;
    NameConstant *tail = NULL;
    NameConstant *nc;

    ret = alloc_statement(AUTO_STATEMENT, line_number);

    for (;;) {
        name_token = get_token();
        check_token(name_token, NAME_TOKEN);
        nc = bcp_malloc(sizeof(NameConstant));
        nc->name = name_token.u.name;
        lb_token = get_token();
        if (lb_token.kind == LB_TOKEN) {
            Constant *c = parse_constant();
            if (c->kind != INT_CONSTANT) {
                bcp_compile_error(VECTOR_SIZE_MUST_BE_AN_INTEGER_ERR,
                                  c->line_number);
            }
            nc->is_vec = TRUE;
            nc->vec_size = c->u.int_value + 1;
            rb_token = get_token();
            check_token(rb_token, RB_TOKEN);
        } else {
            unget_token(lb_token);
            nc->is_vec = FALSE;
        }
        nc->next = NULL;

        if (head == NULL) {
            head = nc;
        } else {
            tail->next = nc;
        }
        tail = nc;

        next_token = get_token();
        check_token2(next_token, COMMA_TOKEN, SEMICOLON_TOKEN);

        if (next_token.kind == SEMICOLON_TOKEN) {
            break;
        }
    }
    ret->u.auto_s.name_constant = head;
    ret->u.auto_s.following = parse_statement();

    return ret;
}

static Statement *
parse_extrn_statement(int line_number)
{
    Statement *ret;
    Token name_token;
    Token comma_token;
    NameItem *name;
    NameItem *head = NULL;
    NameItem *tail = NULL;

    ret = alloc_statement(EXTRN_STATEMENT, line_number);

    for (;;) {
        name_token = get_token();
        check_token(name_token, NAME_TOKEN);

        name = bcp_malloc(sizeof(NameItem));
        name->name = name_token.u.name;
        name->next = NULL;
        if (head == NULL) {
            head = name;
        } else {
            tail->next = name;
        }
        tail = name;

        comma_token = get_token();
        if (comma_token.kind == SEMICOLON_TOKEN) {
            break;
        }
        check_token(comma_token, COMMA_TOKEN);
    }
    ret->u.extrn_s.name_list = head;
    ret->u.extrn_s.following = parse_statement();

    return ret;
}

static Statement *
parse_labeled_statement(char *name, int line_number)
{
    Statement *ret;

    ret = alloc_statement(LABELED_STATEMENT, line_number);
    ret->u.label_s.name = name;
    ret->u.label_s.following = parse_statement();

    return ret;
}

static Statement *parse_expression_statement(int line_number);

static Statement *
parse_starts_with_name_statement(Token name_token)
{
    Token token = get_token();
    Statement *ret;

    if (token.kind == COLON_TOKEN) {
        ret = parse_labeled_statement(name_token.u.name,
                                      name_token.line_number);
    } else {
        unget_token(token);
        unget_token(name_token);
        ret = parse_expression_statement(name_token.line_number);
    }

    return ret;
}

static Statement *
parse_case_statement(int line_number)
{
    Statement *ret;
    Token colon_token;

    ret = alloc_statement(CASE_STATEMENT, line_number);
    ret->u.case_s.expr = constant_to_expression(parse_constant());
    colon_token = get_token();
    check_token(colon_token, COLON_TOKEN);
    ret->u.case_s.following = parse_statement();

    return ret;
}

static Statement *
parse_default_statement(int line_number)
{
    Statement *ret;
    Token colon_token;

    ret = alloc_statement(DEFAULT_STATEMENT, line_number);
    colon_token = get_token();
    check_token(colon_token, COLON_TOKEN);
    ret->u.default_s.following = parse_statement();

    return ret;
}

static Statement *
parse_compound_statement(int line_number)
{
    Statement *ret;
    Token token;
    Statement *head = NULL;
    Statement *tail = NULL;
    Statement *stmt;

    ret = alloc_statement(COMPOUND_STATEMENT, line_number);

    for (;;) {
        token = get_token();
        if (token.kind == RC_TOKEN) {
            break;
        }
        unget_token(token);
        stmt = parse_statement();
        if (head == NULL) {
            head = stmt;
        } else {
            tail->next = stmt;
        }
        for (tail = stmt; tail->next != NULL; tail = tail->next)
            ;
    }
    ret->u.compound_s.stmt = head;

    return ret;
}

static Statement *
parse_if_statement(int line_number)
{
    Statement *ret;
    Token token;

    ret = alloc_statement(IF_STATEMENT, line_number);
    token = get_token();
    check_token(token, LP_TOKEN);
    ret->u.if_s.cond = parse_expression();
    token = get_token();
    check_token(token, RP_TOKEN);

    ret->u.if_s.then_clause = parse_statement();

    token = get_token();
    if (token.kind == ELSE_TOKEN) {
        ret->u.if_s.else_clause = parse_statement();
    } else {
        ret->u.if_s.else_clause = NULL;
        unget_token(token);
    }

    return ret;
}

static Statement *
parse_while_statement(int line_number)
{
    Statement *ret;
    Token token;

    ret = alloc_statement(WHILE_STATEMENT, line_number);
    token = get_token();
    check_token(token, LP_TOKEN);
    ret->u.if_s.cond = parse_expression();
    token = get_token();
    check_token(token, RP_TOKEN);

    ret->u.while_s.stmt = parse_statement();

    return ret;
}

static Statement *
parse_switch_statement(int line_number)
{
    Statement *ret;
    Token token;

    ret = alloc_statement(SWITCH_STATEMENT, line_number);
    token = get_token();
    check_token(token, LP_TOKEN);
    ret->u.switch_s.value = parse_expression();
    token = get_token();
    check_token(token, RP_TOKEN);

    ret->u.switch_s.stmt = parse_statement();
    ret->u.switch_s.case_list = NULL;
    ret->u.switch_s.default_case = NULL;

    return ret;
}

static Statement *
parse_goto_statement(int line_number)
{
    Statement *ret;
    Token semicolon_token;

    ret = alloc_statement(GOTO_STATEMENT, line_number);
    ret->u.goto_s.value = parse_expression();
    semicolon_token = get_token();
    check_token(semicolon_token, SEMICOLON_TOKEN);

    return ret;
}

static Statement *
parse_break_statement(int line_number)
{
    Statement *ret;
    Token semicolon_token;

    ret = alloc_statement(BREAK_STATEMENT, line_number);
    semicolon_token = get_token();
    check_token(semicolon_token, SEMICOLON_TOKEN);

    return ret;
}

static Statement *
parse_return_statement(int line_number)
{
    Statement *ret;
    Token semicolon_token;

    ret = alloc_statement(RETURN_STATEMENT, line_number);
    semicolon_token = get_token();
    if (semicolon_token.kind == SEMICOLON_TOKEN) {
        ret->u.goto_s.value = NULL;
    } else {
        unget_token(semicolon_token);
        ret->u.goto_s.value = parse_expression();
        semicolon_token = get_token();
        check_token(semicolon_token, SEMICOLON_TOKEN);
    }

    return ret;
}

static Statement *
parse_null_statement(int line_number)
{
    Statement *ret;

    ret = alloc_statement(NULL_STATEMENT, line_number);

    return ret;
}

static Statement *
parse_expression_statement(int line_number)
{
    Statement *ret;
    Token semicolon_token;

    ret = alloc_statement(EXPRESSION_STATEMENT, line_number);
    ret->u.expr_s.expr = parse_expression();

    semicolon_token = get_token();
    check_token(semicolon_token, SEMICOLON_TOKEN);

    return ret;
}

static Statement *
parse_statement(void)
{
    Token first_token;
    Statement *ret;

    first_token = get_token();

    if (first_token.kind == AUTO_TOKEN) {
        ret = parse_auto_statement(first_token.line_number);
    } else if (first_token.kind == EXTRN_TOKEN) {
        ret = parse_extrn_statement(first_token.line_number);
    } else if (first_token.kind == NAME_TOKEN) {
        ret = parse_starts_with_name_statement(first_token);
    } else if (first_token.kind == CASE_TOKEN) {
        ret = parse_case_statement(first_token.line_number);
    } else if (first_token.kind == DEFAULT_TOKEN) {
        ret = parse_default_statement(first_token.line_number);
    } else if (first_token.kind == LC_TOKEN) {
        ret = parse_compound_statement(first_token.line_number);
    } else if (first_token.kind == IF_TOKEN) {
        ret = parse_if_statement(first_token.line_number);
    } else if (first_token.kind == WHILE_TOKEN) {
        ret = parse_while_statement(first_token.line_number);
    } else if (first_token.kind == SWITCH_TOKEN) {
        ret = parse_switch_statement(first_token.line_number);
    } else if (first_token.kind == GOTO_TOKEN) {
        ret = parse_goto_statement(first_token.line_number);
    } else if (first_token.kind == BREAK_TOKEN) {
        ret = parse_break_statement(first_token.line_number);
    } else if (first_token.kind == RETURN_TOKEN) {
        ret = parse_return_statement(first_token.line_number);
    } else if (first_token.kind == SEMICOLON_TOKEN) {
        ret = parse_null_statement(first_token.line_number);
    } else {
        unget_token(first_token);
        ret = parse_expression_statement(first_token.line_number);
    }

    return ret;
}

static Definition *
parse_function_definition(char *name)
{
    NameItem *params;
    Statement *stmt;
    Definition *ret;


    params = parse_parameters();
    stmt = parse_statement();

    ret = bcp_malloc(sizeof(Definition));
    ret->kind = FUNCTION_DEFINITION;
    ret->u.func_def.name = name;
    ret->u.func_def.params = params;
    ret->u.func_def.stmt = stmt;
    ret->next = NULL;

    return ret;
}

IVal *
bcp_alloc_ival(IValKind kind)
{
    IVal *p;

    p = bcp_malloc(sizeof(IVal));
    p->kind = kind;
    p->next = NULL;

    return p;
}

static IVal *
parse_ival_list(void)
{
    Token token;
    IVal *head = NULL;
    IVal *tail = NULL;
    IVal *ival;
    
    for (;;) {
        token = get_token();
        if (token.kind == SEMICOLON_TOKEN) {
            break;
        } else if (token.kind == NAME_TOKEN) {
            ival = bcp_alloc_ival(NAME_IVAL);
            ival->u.n.name = token.u.name;
        } else {
            unget_token(token);
            ival = bcp_alloc_ival(CONSTANT_IVAL);
            ival->u.c.constant = parse_constant();
        }
        if (head == NULL) {
            head = ival;
        } else {
            tail->next = ival;
        }
        tail = ival;
        token = get_token();
        if (token.kind == SEMICOLON_TOKEN) {
            break;
        }
        check_token(token, COMMA_TOKEN);
    }

    return head;
}

static Definition *
parse_declaration(char *name)
{
    Token token;
    Definition *def = bcp_malloc(sizeof(Definition));
    Constant *c;

    def->kind = DECLARATION_DEFINITION;
    def->u.decl_def.name = name;
    def->next = NULL;

    token = get_token();
    if (token.kind == LB_TOKEN) {
        def->u.decl_def.is_vec = TRUE;

        token = get_token();
        if (token.kind == RB_TOKEN) {
            def->u.decl_def.has_vec_size = FALSE;
        } else {
            unget_token(token);
            def->u.decl_def.has_vec_size = TRUE;
            c = parse_constant();
            if (c->kind != INT_CONSTANT) {
                bcp_compile_error(VECTOR_SIZE_MUST_BE_AN_INTEGER_ERR,
                                  c->line_number);
            }
            def->u.decl_def.vec_size = c->u.int_value + 1;
            token = get_token();
            check_token(token, RB_TOKEN);
        }
        def->u.decl_def.ival_list = parse_ival_list();
    } else {
        unget_token(token);
        def->u.decl_def.is_vec = FALSE;
        def->u.decl_def.ival_list = parse_ival_list();
    }
    return def;
}

static Definition *
parse_definition(void)
{
    Token name_token;
    Token next_token;
    Definition *ret;

    name_token = get_token();
    if (name_token.kind == END_OF_FILE_TOKEN) {
        return NULL;
    }
    check_token(name_token, NAME_TOKEN);

    next_token = get_token();
    if (next_token.kind == LP_TOKEN) {
        ret = parse_function_definition(name_token.u.name);
    } else {
        unget_token(next_token);
        ret = parse_declaration(name_token.u.name);
    }
    ret->line_number = name_token.line_number;

    return ret;
}

static Definition *
parse(void)
{
    Definition *head = NULL;
    Definition *tail = NULL;
    Definition *def;

    for (;;) {
        def = parse_definition();
        if (def == NULL) {
            return head;
        }
        if (head == NULL) {
            head = def;
        } else {
            tail->next = def;
        }
        tail = def;
    }
    return head;
}

int *
BCP_compile(FILE *src_fp, int *main_address, int *heap_start_address)
{
    Definition *def_head;
    ParseTree *parse_tree;
    MEM_Storage storage;
    int *memory;
    int i;

    storage = MEM_open_storage(4096);
    bcp_lex_initialize(storage, src_fp);
    def_head = parse();
    parse_tree = bcp_fix_tree(def_head);

    memory = bcp_generate_code(parse_tree);
    for (i = 0; i < parse_tree->static_name_count; i++) {
        if (!strcmp(parse_tree->static_name[i].name, "main")) {
            *main_address = parse_tree->static_name[i].address;
            break;
        }
    }
    if (i == parse_tree->static_name_count) {
        bcp_compile_error(NAME_NOT_FOUND_ERR, 0, "main");
    }
    *heap_start_address = parse_tree->heap_start_address;

    /*
    bcp_dump_tree(stdout, parse_tree, memory);
    */

    return memory;
    /*
    for (;;) {
        token = bcp_get_token();
        if (token.kind == END_OF_FILE_TOKEN) {
            break;
        }
        if (token.kind == NAME_TOKEN) {
            printf("%d:token..%s (%s)\n", token.line_number,
                   bcp_token_str[token.kind], token.u.name);
        } else if (token.kind == INT_LITERAL_TOKEN) {
            printf("%d:token..%s (%d)\n", token.line_number,
                   bcp_token_str[token.kind],
                   token.u.int_value);
        } else if (token.kind == STRING_LITERAL_TOKEN) {
            printf("%d:token..%s (%s)\n", token.line_number,
                   bcp_token_str[token.kind],
                   token.u.str_literal.str);
        } else if (token.kind == CHARS_TOKEN) {
            printf("%d:token..%s (%08x)\n", token.line_number,
                   bcp_token_str[token.kind],
                   token.u.chars);
        } else {
            printf("%d:token..%s\n", token.line_number,
                   bcp_token_str[token.kind]);
        }
    }
    */

}

