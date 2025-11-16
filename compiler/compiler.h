#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED
#include "BCP.h"
#include "token.h"

typedef enum {
    FALSE,
    TRUE
} Boolean;

typedef struct NameItem_tag {
    char *name;
    struct NameItem_tag *next;
} NameItem;

typedef enum {
    INT_CONSTANT,
    CHARS_CONSTANT,
    STRING_CONSTANT
} ConstantKind;

typedef struct {
    ConstantKind kind;
    union {
	int int_value;
	StringLiteral str_literal;
	unsigned int chars;
    } u;
    int line_number;
} Constant;

typedef enum {
    NAME_EXPRESSION,
    INTEGER_LITERAL,
    CHARS_LITERAL,
    STRING_LITERAL,
    INDEX_EXPRESSION,
    FUNCTION_CALL_EXPRESSION,
    UNARY_EXPRESSION,
    BINARY_EXPRESSION,
    CONDITIONAL_EXPRESSION
} ExpressionKind;

typedef struct Expression_tag Expression;

typedef struct {
    char *name;
} NameExpression;

typedef struct {
    int int_value;
} IntegerLiteral;

typedef struct {
    unsigned int chars;
} CharsLiteral;

typedef struct {
    StringLiteral str_literal;
} StringLiteralExpression;

typedef struct {
    Expression *vector;
    Expression *index;
} IndexExpression;

typedef struct Argument_tag {
    Expression *expr;
    struct Argument_tag *next;
} Argument;

typedef struct {
    Expression *func;
    Argument *args;
} FunctionCallExpression;

typedef enum {
    INDIRECTION_OPERATOR,
    ADDRESS_OPERATOR,
    MINUS_OPERATOR,
    LOGICAL_NOT_OPERATOR,
    PRE_INC_OPERATOR,
    PRE_DEC_OPERATOR,
    POST_INC_OPERATOR,
    POST_DEC_OPERATOR,
    BIT_NOT_OPERATOR
} UnaryOperator;

typedef struct {
    UnaryOperator operator;
    Expression *operand;
} UnaryExpression;

typedef enum {
    MUL_OPERATOR,
    DIV_OPERATOR,
    MOD_OPERATOR,
    ADD_OPERATOR,
    SUB_OPERATOR,
    LEFT_SHIFT_OPERATOR,
    RIGHT_SHIFT_OPERATOR,
    LT_OPERATOR,
    LE_OPERATOR,
    GT_OPERATOR,
    GE_OPERATOR,
    EQ_OPERATOR,
    NE_OPERATOR,
    BIT_AND_OPERATOR,
    BIT_XOR_OPERATOR,
    BIT_OR_OPERATOR,
    ASSIGN_OPERATOR,
    ADD_ASSIGN_OPERATOR,
    SUB_ASSIGN_OPERATOR,
    MUL_ASSIGN_OPERATOR,
    DIV_ASSIGN_OPERATOR,
    MOD_ASSIGN_OPERATOR,
    LEFT_SHIFT_ASSIGN_OPERATOR,
    RIGHT_SHIFT_ASSIGN_OPERATOR,
    LT_ASSIGN_OPERATOR,
    LE_ASSIGN_OPERATOR,
    GT_ASSIGN_OPERATOR,
    GE_ASSIGN_OPERATOR,
    EQ_ASSIGN_OPERATOR,
    NE_ASSIGN_OPERATOR,
    BIT_AND_ASSIGN_OPERATOR,
    BIT_XOR_ASSIGN_OPERATOR,
    BIT_OR_ASSIGN_OPERATOR
} BinaryOperator;

typedef struct {
    BinaryOperator operator;
    Expression *left;
    Expression *right;
} BinaryExpression;

typedef struct {
    Expression *cond;
    Expression *expr1;
    Expression *expr2;
} ConditionalExpression;

struct Expression_tag {
    ExpressionKind kind;
    int line_number;
    union {
	NameExpression name_e;
	IntegerLiteral int_e;
	CharsLiteral chars_e;
	StringLiteralExpression str_e;
	IndexExpression index_e;
	FunctionCallExpression func_call_e;
	UnaryExpression unary_e;
	BinaryExpression binary_e;
	ConditionalExpression cond_e;
    } u;
};

typedef enum {
    AUTO_STATEMENT,
    EXTRN_STATEMENT,
    LABELED_STATEMENT,
    CASE_STATEMENT,
    DEFAULT_STATEMENT,
    COMPOUND_STATEMENT,
    IF_STATEMENT,
    WHILE_STATEMENT,
    SWITCH_STATEMENT,
    GOTO_STATEMENT,
    BREAK_STATEMENT,
    RETURN_STATEMENT,
    EXPRESSION_STATEMENT
} StatementKind;

typedef struct Statement_tag Statement;

typedef struct NameConstant_tag {
    char *name;
    Constant *vec_size;
    struct NameConstant_tag *next;
} NameConstant;

typedef struct {
    NameConstant *name_constant;
    Statement *following;
} AutoStatement;

typedef struct {
    NameItem *name_list;
    Statement *following;
} ExtrnStatement;

typedef struct {
    char *name;
    Statement *following;
} LabeledStatement;

typedef struct {
    Constant *constant;
    Statement *following;
} CaseStatement;

typedef struct {
    Statement *following;
} DefaultStatement;

typedef struct {
    Statement *stmt;
} CompoundStatement;

typedef struct {
    Expression *cond;
    Statement *then_clause;
    Statement *else_clause;
} IfStatement;

typedef struct {
    Expression *cond;
    Statement *stmt;
} WhileStatement;

typedef struct {
    Expression *value;
    Statement *stmt;
} SwitchStatement;

typedef struct {
    Expression *value;
} GotoStatement;

typedef struct {
    Expression *value;
} ReturnStatement;

typedef struct {
    Expression *expr;
} ExpressionStatement;

struct Statement_tag {
    StatementKind kind;
    int line_number;
    union {
	AutoStatement auto_s;
	ExtrnStatement extrn_s;
	LabeledStatement label_s;
	CaseStatement case_s;
	DefaultStatement default_s;
	CompoundStatement compound_s;
	IfStatement if_s;
	WhileStatement while_s;
	SwitchStatement switch_s;
	GotoStatement goto_s;
	ReturnStatement return_s;
	ExpressionStatement expr_s;
    } u;
    struct Statement_tag *next;
};

typedef enum {
    FUNCTION_DEFINITION,
    DECLARATION_DEFINITION
} DefinitionKind;

typedef struct {
    char *name;
    NameItem *params;
    Statement *stmt;
} FunctionDefinition;

typedef enum {
    CONSTANT_IVAL,
    NAME_IVAL
} IValKind;

typedef struct IVal_tag {
    IValKind kind;
    union {
	Constant *constant;
	char *name;
    } u;
    struct IVal_tag *next;
} IVal;

typedef struct {
    char *name;
    Boolean is_vec;
    Boolean has_vec_size;
    int vec_size;
    IVal *ival_list;
} DeclarationDefinition;

typedef struct Definition_tag {
    DefinitionKind kind;
    union {
	FunctionDefinition func_def;
	DeclarationDefinition decl_def;
    } u;
    struct Definition_tag *next;
} Definition;

typedef enum {
    BAD_STRING_ESCAPE_SEQUENCE_ERR,
    CHARACTER_CONSTANT_TOO_LONG_ERR,
    EOF_IN_STRING_LITERAL_ERR,
    LF_IN_STRING_LITERAL_ERR,
    EOF_IN_CHARS_ERR,
    LF_IN_CHARS_ERR,
    EOF_IN_COMMENT_ERR,
    UNEXPECTED_TOKEN0_ERR,
    UNEXPECTED_TOKEN_ERR,
    UNEXPECTED_TOKEN2_ERR,
    NOT_OCTAL_ERR,
    INVALID_CHARACTER_ERR,
    VECTOR_SIZE_MUST_BE_AN_INTEGER_ERR
} CompileErrorCode;

/* lexer.c */
void *bcp_malloc(size_t size);
void bcp_lex_initialize(MEM_Storage storage, FILE *src_fp);
Token bcp_get_token(void);
extern char *bcp_token_str[];

/*dump_tree.c */
void bcp_dump_tree(FILE *fp, Definition *def_head);

/* message.c */
void bcp_compile_error(CompileErrorCode code, int line_number, ...);

#endif /* COMPILER_H_INCLUDED */


