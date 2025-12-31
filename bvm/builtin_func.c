#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "BVM.h"
#include "bvm_pri.h"

static FILE *st_current_input;
static FILE *st_current_output;

#define CHAR_IN_INT(array, n) (\
        ((array)[(n) / sizeof(int)] \
         >> ((sizeof(int) - 1 - ((n) % sizeof(int))) * CHAR_BIT)) & 0xff)

static void
put_str(FILE *fp, int *str)
{
    int ch;
    int i;

    for (i = 0; ; i++) {
        ch = CHAR_IN_INT(str, i);
        if (ch == BVM_EOT) {
            break;
        }
        fputc(ch, fp);
    }
}

static int
fun_printf(int argc, int *args, int *memory)
{
    int *fmt;
    int fmt_ch;
    int *values;
    int fmt_idx;
    int val_idx;

    fmt = &memory[args[0]];
    values = &args[1];
    val_idx = 0;

    for (fmt_idx = 0; ; fmt_idx++) {
        fmt_ch = CHAR_IN_INT(fmt, fmt_idx);
        if (fmt_ch == BVM_EOT) {
            break;
        }

        if (fmt_ch == '%') {
            if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'c') {
                fprintf(st_current_output, "%c", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'd') {
                fprintf(st_current_output, "%d", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 'o') {
                fprintf(st_current_output, "%o", values[val_idx]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == 's') {
                put_str(st_current_output, &memory[values[val_idx]]);
                val_idx++;
            } else if (CHAR_IN_INT(fmt, fmt_idx + 1) == '%') {
                fputc('%', st_current_output);
            }
            fmt_idx++;
        } else {
            fputc(fmt_ch, st_current_output);
        }
    }
    return val_idx;
}

static int
fun_putnumb(int argc, int *args, int *memory)
{
    return fprintf(st_current_output, "%d", args[0]);
}

static int
fun_putchar(int argc, int *args, int *memory)
{
    int i;
    int ch;

    for (i = 0; i < sizeof(int); i++) {
        ch = (((unsigned int*)args)[0] >> ((sizeof(int) - i - 1)) * CHAR_BIT)
            & 0xff;
        if (ch != 0 && ch != BVM_EOT) {
            fputc(ch, st_current_output);
        } else if (ch == BVM_EOT) {
            break;
        }
    }

    return ch;
}

static int
fun_getchar(int argc, int *args, int *memory)
{
    int ret;
    ret = fgetc(st_current_input);

    if (ret == EOF) {
        return BVM_EOT;
    } else {
        return ret;
    }
}

static int
fun_char(int argc, int *args, int *memory)
{
    int s = args[0];
    int n = args[1];
    unsigned int y; /* word containing n-th char */
    int sh, cpos;
    
    y = ((unsigned int*)memory)[s + n / sizeof(int)];
    cpos = n % sizeof(int);        /* position of char in word */
    sh = (sizeof(int) - 1 - cpos) * CHAR_BIT; /* bit positions to shift */
    y =  (y >> sh) & 0xff;    /* shift and mask all but 8 bits */

   return y;         /* return character to caller */
}

static void
lchar(int s, int n, int c, int *memory)
{
    int y, cpos, sh, mask;

    y = memory[s + n / sizeof(int)];  /* word containing n-th char */
    cpos = n % sizeof(int);        /* position of char in word */
    sh = (sizeof(int) - 1 - cpos) * CHAR_BIT; /* bit positions to shift */
    mask = 0xff << sh;
    
    memory[s + n / sizeof(int)] =  (y & ~mask) | (c << sh);
}

static int
fun_lchar(int argc, int *args, int *memory)
{
    int s = args[0];
    int n = args[1];
    int c = args[2];

    lchar(s, n, c, memory);

    return c;
}

static int
fun_getstr(int argc, int *args, int *memory)
{
    int s = args[0];
    int c;
    int i;

    i = 0;
    while ((c = fgetc(st_current_input)) != '\n') {
        lchar(s, i++, c, memory);
    }
    lchar(s, i, BVM_EOT, memory);

    return s;
}

static int
fun_putstr(int argc, int *args, int *memory)
{
    int s = args[0];

    put_str(st_current_output, &memory[s]);

    return 0;
}

static int
fun_nargs(int argc, int *args, int *memory)
{
    return bvm_get_current_arg_count();
}

static int
fun_exit(int argc, int *args, int *memory)
{
    exit(0);
}

static char *st_builtin_function_names[] = {
    "printf",
    "putnumb",
    "putchar",
    "getchar",
    "char",
    "lchar",
    "getstr",
    "putstr",
    "nargs",
    "exit",
};

BuiltinFunction
bvm_builtin_functions[] = {
    {"printf", fun_printf},
    {"putnumb", fun_putnumb},
    {"putchar", fun_putchar},
    {"getchar", fun_getchar},
    {"char", fun_char},
    {"lchar", fun_lchar},
    {"getstr", fun_getstr},
    {"putstr", fun_putstr},
    {"nargs", fun_nargs},
    {"exit", fun_exit},
};

int
bvm_get_builtin_function_count(void)
{
    int count1 = sizeof(st_builtin_function_names) / sizeof(char*);
    int count2 = sizeof(bvm_builtin_functions) / sizeof(BuiltinFunction);
    
    assert(count1 == count2);

    return count1;
}

char **BVM_get_builtin_functions(int *count)
{
    *count = sizeof(st_builtin_function_names) / sizeof(char*);

    return st_builtin_function_names;
}

void
bvm_init_builtin_function(void)
{
    st_current_input = stdin;
    st_current_output = stdout;
}

#if 0
int
bvm_fun_print(int *args, int *memory)
{
    printf("%d\n", args[0]);

    return 8;
}

int
bvm_fun_print_s(int *args, int *memory)
{
    printf("%s\n", (char*)&memory[args[0]]);

    return 15;
}
#endif
