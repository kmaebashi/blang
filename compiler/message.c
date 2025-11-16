#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "compiler.h"

static char *st_messages[] = {
    "bad string escape sequence.(%c)",
    "character constant too long.(%d)",
    "EOF in string literal.",
    "LF in string literal.",
    "EOF in chars.",
    "LF in chars.",
    "EOF in comment.",
    "unexpected token %s.",
    "unexpected token %s. %s is expected.",
    "unexpected token %s. %s or %s is expected.",
    "not octal. (%s)",
    "invalid character.",
    "vector size must be an integer.(%s)",
};

void
bcp_compile_error(CompileErrorCode code, int line_number, ...)
{
    va_list argp;
    char *fmt = st_messages[code];

    fprintf(stderr, "%d:", line_number);
    va_start(argp, line_number);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    fprintf(stderr, "\n");

    exit(1);
}
