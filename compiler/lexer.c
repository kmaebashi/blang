#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "MEM.h"
#include "BVM.h"
#include "token.h"
#include "compiler.h"

static FILE *st_source_file;

static signed char *st_token_buffer;
static int  st_token_buffer_alloc_size;
static int  st_token_length;
static int  st_current_line_number;
static MEM_Storage st_mem_storage;

#define TOKEN_BUFFER_ALLOC_SIZE (256)

char *bcp_token_str[] = {
    "IF",
    "ELSE",
    "WHILE",
    "SWITCH",
    "CASE",
    "DEFAULT",
    "GOTO",
    "BREAK",
    "RETURN",
    "EXTRN",
    "AUTO",
    "LP",
    "RP",
    "LC",
    "RC",
    "LB",
    "RB",
    "AMPERSAND",
    "QUESTION",
    "COLON",
    "SEMICOLON",
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
    "BIT_OR_ASSIGN",
    "BIT_XOR_ASSIGN",
    "INC",
    "DEC",
    "MINUS",
    "EXCLAMATION",
    "BIT_OR",
    "BIT_XOR",
    "BIT_NOT",
    "EQ",
    "NE",
    "LT",
    "LE",
    "GT",
    "GE",
    "LEFT_BIT_SHIFT",
    "RIGHT_BIT_SHIFT",
    "PLUS",
    "PERCENT",
    "ASTERISK",
    "SLASH",
    "COMMA",
    "NAME",
    "INT_LITERAL",
    "STRING_LITERAL",
    "CHARS",
    "END_OF_FILE",
};

void *
bcp_malloc(size_t size)
{
    char *p = MEM_storage_malloc(st_mem_storage, size);

    return p;
}

static char *
lex_strdup(char *src)
{
    char *ret = bcp_malloc(strlen(src) + 1);
    strcpy(ret, src);

    return ret;
}

static unsigned int
byte_array_to_int(signed char *array, int size)
{
    int i;
    unsigned int ret = 0;

    for (i = 0; i < size; i++) {
        ret += array[size - 1 - i] << (i * CHAR_BIT);
    }

    return ret;
}

typedef struct Keyword_tag {
    char *name;
    TokenKind token;
} Keyword;

Keyword st_keywords[] = {
    {"if", IF_TOKEN},
    {"else", ELSE_TOKEN},
    {"while", WHILE_TOKEN},
    {"switch", SWITCH_TOKEN},
    {"case", CASE_TOKEN},
    {"default", DEFAULT_TOKEN},
    {"goto", GOTO_TOKEN},
    {"break", BREAK_TOKEN},
    {"return", RETURN_TOKEN},
    {"extrn", EXTRN_TOKEN},
    {"auto", AUTO_TOKEN},
};

static int
is_keyword(char *name, TokenKind *token_kind) {
    int i;

    for (i = 0; i < sizeof(st_keywords) / sizeof(Keyword); i++) {
        if (!strcmp(st_keywords[i].name, (char*)name)) {
            *token_kind = st_keywords[i].token;
            return 1;
        }
    }
    return 0;
}

void
bcp_lex_initialize(MEM_Storage storage, FILE *src_fp)
{
    st_mem_storage = storage;
    st_source_file = src_fp;
    st_current_line_number = 1;
}

static void
add_letter_to_token(int letter)
{
    if (st_token_buffer_alloc_size <= st_token_length + 1) {
        st_token_buffer_alloc_size += TOKEN_BUFFER_ALLOC_SIZE;
        st_token_buffer = realloc(st_token_buffer,
                            st_token_buffer_alloc_size);
    }
    st_token_buffer[st_token_length] = letter;
    st_token_buffer[st_token_length + 1] = '\0';
    st_token_length++;
}

static int escape_seq_to_int(int ch) {
    int ret = 0; /* make compiler happy */

    if (ch == '0') {
        ret = 0;
    } else if (ch == 'e') {
        ret = BVM_EOT;
    } else if (ch == '(') {
        ret = '{';
    } else if (ch == ')') {
        ret = '}';
    } else if (ch == 't') {
        ret = '\t';
    } else if (ch == '*') {
        ret = '*';
    } else if (ch == '\'') {
        ret = '\'';
    } else if (ch == '\"') {
        ret = '\"';
    } else if (ch == 'n') {
        ret = '\n';
    } else {
        bcp_compile_error(BAD_STRING_ESCAPE_SEQUENCE_ERR,
                          st_current_line_number,ch);
    }
    return ret;
}


typedef struct {
    char        *token;
    TokenKind   kind;
} OperatorInfo;

static OperatorInfo st_operator_table[] = {
    {"=<<", LEFT_SHIFT_ASSIGN_TOKEN},
    {"=>>", RIGHT_SHIFT_ASSIGN_TOKEN},
    {"=<=", LE_ASSIGN_TOKEN},
    {"=>=", GE_ASSIGN_TOKEN},
    {"===", EQ_ASSIGN_TOKEN},
    {"=!=", NE_ASSIGN_TOKEN},
    {"++", INC_TOKEN},
    {"--", DEC_TOKEN},
    {"==", EQ_TOKEN},
    {"!=", NE_TOKEN},
    {"<=", LE_TOKEN},
    {">=", GE_TOKEN},
    {"<<", LEFT_BIT_SHIFT_TOKEN},
    {">>", RIGHT_BIT_SHIFT_TOKEN},
    {"=+", ADD_ASSIGN_TOKEN},
    {"=-", SUB_ASSIGN_TOKEN},
    {"=*", MUL_ASSIGN_TOKEN},
    {"=/", DIV_ASSIGN_TOKEN},
    {"=%", MOD_ASSIGN_TOKEN},
    {"=<", LT_ASSIGN_TOKEN},
    {"=>", GT_ASSIGN_TOKEN},
    {"=&", BIT_AND_ASSIGN_TOKEN},
    {"=|", BIT_OR_ASSIGN_TOKEN},
    {"(", LP_TOKEN},
    {")", RP_TOKEN},
    {"{", LC_TOKEN},
    {"}", RC_TOKEN},
    {"[", LB_TOKEN},
    {"]", RB_TOKEN},
    {"&", AMPERSAND_TOKEN},
    {"?", QUESTION_TOKEN},
    {":", COLON_TOKEN},
    {";", SEMICOLON_TOKEN},
    {"=", ASSIGN_TOKEN},
    {"-", MINUS_TOKEN},
    {"!", EXCLAMATION_TOKEN},
    {"|", BIT_OR_TOKEN},
    {"^", BIT_XOR_TOKEN},
    {"~", BIT_NOT_TOKEN},
    {"<", LT_TOKEN},
    {">", GT_TOKEN},
    {"+", PLUS_TOKEN},
    {"%", PERCENT_TOKEN},
    {"*", ASTERISK_TOKEN},
    {"/", SLASH_TOKEN},
    {",", COMMA_TOKEN},
};

int
is_operator_start_letter(int ch)
{
    int i;
    for (i = 0; i < (sizeof(st_operator_table) / sizeof(OperatorInfo)); i++) {
        if (ch == st_operator_table[i].token[0])
            return 1;
    }
    return 0;
}

int
in_operator(int letter)
{
    int op_idx;
    int letter_idx;

    for (op_idx = 0;
         op_idx < (sizeof(st_operator_table) / sizeof(OperatorInfo));
         op_idx++) {
        for (letter_idx = 0;
             st_token_buffer[letter_idx] != '\0'
                 && st_operator_table[op_idx].token[letter_idx] != '\0';
             letter_idx++) {
            if (st_token_buffer[letter_idx]
                != st_operator_table[op_idx].token[letter_idx]) {
                break;
            }
        }
        if (st_token_buffer[letter_idx] == '\0'
            && st_operator_table[op_idx].token[letter_idx] == letter) {
            return 1;
        }
    }
    return 0;
}

typedef enum {
    INITIAL_STATUS,
    INT_STATUS,
    NAME_STATUS,
    STRING_STATUS,
    ASTERISK_IN_STRING_STATUS,
    CHARS_STATUS,
    ASTERISK_IN_CHARS_STATUS,
    IN_OPERATOR_STATUS,
    SLASH_STATUS,
    IN_COMMENT_STATUS,
    ASTERISK_IN_COMMENT_STATUS
} LexerStatus;

Token
bcp_get_token(void)
{
    Token ret;
    LexerStatus status = INITIAL_STATUS;
    int ch;
    
    st_token_length = 0;
    for (;;) {
        ch = getc(st_source_file);
        switch (status) {
        case INITIAL_STATUS:
            if (isdigit(ch)) {
                add_letter_to_token(ch);
                status = INT_STATUS;
            } else if (isalpha(ch) || ch == '_') {
                add_letter_to_token(ch);
                status = NAME_STATUS;
            } else if (ch == '"') {
                status = STRING_STATUS;
            } else if (ch == '\'') {
                status = CHARS_STATUS;
            } else if (ch == '/') {
                status = SLASH_STATUS;
            } else if (is_operator_start_letter(ch)) {
                add_letter_to_token(ch);
                status = IN_OPERATOR_STATUS;
            } else if (ch == EOF) {
                ret.kind = END_OF_FILE_TOKEN;
                goto LOOP_END;
            } else if (isspace(ch)) {
                if (ch == '\n') {
                    st_current_line_number++;
                }
            } else {
                bcp_compile_error(INVALID_CHARACTER_ERR,
                                  st_current_line_number);
            }
            break;
        case INT_STATUS:
            if (isdigit(ch)) {
                add_letter_to_token(ch);
            } else {
                ret.kind = INT_LITERAL_TOKEN;
                if (st_token_buffer[0] == '0') {
                    if (st_token_length == 1) {
                        ret.u.int_value = 0;
                    } else {
                        int sscanf_ret
                            = sscanf((char*)st_token_buffer + 1,
                                     "%o",(unsigned int*)&ret.u.int_value);
                        if (sscanf_ret != 1) {
                            bcp_compile_error(NOT_OCTAL_ERR,
                                              st_current_line_number,
                                              st_token_buffer);
                        }
                    }
                } else {
                    sscanf((char*)st_token_buffer, "%d",&ret.u.int_value);
                }
                ungetc(ch, st_source_file);
                goto LOOP_END;
            }
            break;
        case NAME_STATUS:
            if (isalnum(ch) || ch == '_') {
                add_letter_to_token(ch);
            } else {
                TokenKind token_kind;

                ungetc(ch, st_source_file);

                if (is_keyword((char*)st_token_buffer, &token_kind)) {
                    ret.kind = token_kind;
                } else {
                    ret.kind = NAME_TOKEN;
                    ret.u.name = lex_strdup((char*)st_token_buffer);
                }
                goto LOOP_END;
            }
            break;
        case STRING_STATUS:
            if (ch == '*') {
                status = ASTERISK_IN_STRING_STATUS;
            } else if (ch == '\"') {
                ret.kind = STRING_LITERAL_TOKEN;
                ret.u.str_literal.length = st_token_length;
                ret.u.str_literal.str = bcp_malloc(st_token_length + 1);
                memcpy(ret.u.str_literal.str, st_token_buffer,
                       st_token_length + 1);
                goto LOOP_END;
            } else if (ch == '\n') {
                bcp_compile_error(LF_IN_STRING_LITERAL_ERR,
                                  st_current_line_number);
            } else if (ch == EOF) {
                bcp_compile_error(EOF_IN_STRING_LITERAL_ERR,
                                  st_current_line_number);
            } else {
                add_letter_to_token(ch);
            }
            break;
        case ASTERISK_IN_STRING_STATUS:
            add_letter_to_token(escape_seq_to_int(ch));
            status = STRING_STATUS;
            break;
        case CHARS_STATUS:
            if (ch == '*') {
                status = ASTERISK_IN_CHARS_STATUS;
            } else if (ch == '\'') {
                ret.kind = CHARS_TOKEN;
                if (st_token_length > sizeof(int)) {
                    bcp_compile_error(CHARACTER_CONSTANT_TOO_LONG_ERR,
                                      st_token_length);
                }
                ret.u.chars
                    = byte_array_to_int(st_token_buffer, st_token_length);
                goto LOOP_END;
            } else if (ch == '\n') {
                bcp_compile_error(LF_IN_CHARS_ERR, st_current_line_number);
            } else if (ch == EOF) {
                bcp_compile_error(EOF_IN_CHARS_ERR, st_current_line_number);
            } else {
                add_letter_to_token(ch);
            }
            break;
        case ASTERISK_IN_CHARS_STATUS:
            add_letter_to_token(escape_seq_to_int(ch));
            status = CHARS_STATUS;
            break;
        case IN_OPERATOR_STATUS:
            if (in_operator(ch)) {
                add_letter_to_token(ch);
            } else {
                int i;
                for (i = 0;
                     i < sizeof(st_operator_table) / sizeof(OperatorInfo);
                     i++) {
                    if (!strcmp(st_operator_table[i].token,
                                (char*)st_token_buffer)) {
                        ungetc(ch, st_source_file);
                        ret.kind = st_operator_table[i].kind;
                        goto LOOP_END;
                    }
                }
            }
            break;
        case SLASH_STATUS:
            if (ch == '*') {
                status = IN_COMMENT_STATUS;
            } else {
                add_letter_to_token(ch);
                ret.kind = SLASH_TOKEN;
                goto LOOP_END;
            }
            break;
        case IN_COMMENT_STATUS:
            if (ch == '*') {
                status = ASTERISK_IN_COMMENT_STATUS;
            } else if (ch == '\n') {
                st_current_line_number++;
            } else if (ch == EOF) {
                bcp_compile_error(EOF_IN_COMMENT_ERR, st_current_line_number);
            }
            break;
        case ASTERISK_IN_COMMENT_STATUS:
            if (ch == '/') {
                status = INITIAL_STATUS;
            } else if (ch == EOF) {
                bcp_compile_error(EOF_IN_COMMENT_ERR, st_current_line_number);
            }
            break;
        }
    }
    ret.kind = END_OF_FILE_TOKEN;

LOOP_END:
    ret.line_number = st_current_line_number;

    return ret;
}

#if 0

int
main(int argc, char **argv)
{

    FILE *src_fp;
    Token       token;

    st_mem_storage = MEM_open_storage(4096);
    src_fp = fopen("sample.b", "r");
    lex_initialize(src_fp);

    for (;;) {
        token = lex_get_token();
        if (token.kind == END_OF_FILE_TOKEN) {
            break;
        }
        if (token.kind == NAME_TOKEN) {
            printf("token..%s (%s)\n", st_token_str[token.kind], token.u.name);
        } else if (token.kind == INT_LITERAL_TOKEN) {
            printf("token..%s (%d)\n", st_token_str[token.kind],
                   token.u.int_value);
        } else if (token.kind == STRING_LITERAL_TOKEN) {
            printf("token..%s (%s)\n", st_token_str[token.kind],
                   token.u.str_literal.str);
        } else if (token.kind == CHARS_TOKEN) {
            printf("token..%s (%08x)\n", st_token_str[token.kind],
                   token.u.chars);
        } else {
            printf("token..%s\n", st_token_str[token.kind]);
        }
    }

    return 0;
}
#endif
