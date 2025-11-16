#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED
#include <stdio.h>

typedef enum {
    IF_TOKEN,
    ELSE_TOKEN,
    WHILE_TOKEN,
    SWITCH_TOKEN,
    CASE_TOKEN,
    DEFAULT_TOKEN,
    GOTO_TOKEN,
    BREAK_TOKEN,
    RETURN_TOKEN,
    EXTRN_TOKEN,
    AUTO_TOKEN,
    LP_TOKEN,
    RP_TOKEN,
    LC_TOKEN,
    RC_TOKEN,
    LB_TOKEN,
    RB_TOKEN,
    AMPERSAND_TOKEN,
    QUESTION_TOKEN,
    COLON_TOKEN,
    SEMICOLON_TOKEN,
    ASSIGN_TOKEN,
    ADD_ASSIGN_TOKEN,
    SUB_ASSIGN_TOKEN,
    MUL_ASSIGN_TOKEN,
    DIV_ASSIGN_TOKEN,
    MOD_ASSIGN_TOKEN,
    LEFT_SHIFT_ASSIGN_TOKEN,
    RIGHT_SHIFT_ASSIGN_TOKEN,
    LT_ASSIGN_TOKEN,
    LE_ASSIGN_TOKEN,
    GT_ASSIGN_TOKEN,
    GE_ASSIGN_TOKEN,
    EQ_ASSIGN_TOKEN,
    NE_ASSIGN_TOKEN,
    BIT_AND_ASSIGN_TOKEN,
    BIT_OR_ASSIGN_TOKEN,
    BIT_XOR_ASSIGN_TOKEN,
    INC_TOKEN,
    DEC_TOKEN,
    MINUS_TOKEN,
    EXCLAMATION_TOKEN,
    BIT_OR_TOKEN,
    BIT_XOR_TOKEN,
    BIT_NOT_TOKEN,
    EQ_TOKEN,
    NE_TOKEN,
    LT_TOKEN,
    LE_TOKEN,
    GT_TOKEN,
    GE_TOKEN,
    LEFT_BIT_SHIFT_TOKEN,
    RIGHT_BIT_SHIFT_TOKEN,
    PLUS_TOKEN,
    PERCENT_TOKEN,
    ASTERISK_TOKEN,
    SLASH_TOKEN,
    COMMA_TOKEN,
    NAME_TOKEN,
    INT_LITERAL_TOKEN,
    STRING_LITERAL_TOKEN,
    CHARS_TOKEN,
    END_OF_FILE_TOKEN
} TokenKind;

typedef struct {
    int length;
    signed char *str;
} StringLiteral;

typedef struct {
    TokenKind kind;
    union {
	int int_value;
	char *name;
	StringLiteral str_literal;
	unsigned int chars;
    } u;
    int line_number;
} Token;

#endif /* TOKEN_H_INCLUDED */
