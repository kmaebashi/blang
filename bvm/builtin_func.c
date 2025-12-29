#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include "BVM.h"
#include "bvm_pri.h"

static FILE *st_current_input;
static FILE *st_current_output;

static void
put_str(FILE *fp, char *str)
{
    int i;

    for (i = 0; str[i] != BVM_EOT; i++) {
        fputc(str[i], fp);
    }
}

static int
fun_printf(int argc, int *args, int *memory)
{
    char *fmt;
    int *values;
    int fmt_idx;
    int val_idx;

    fmt = (char*)&memory[args[0]];
    values = &args[1];
    val_idx = 0;

    for (fmt_idx = 0; fmt[fmt_idx] != BVM_EOT; fmt_idx++) {
        if (fmt[fmt_idx] == '%') {
            if (fmt[fmt_idx + 1] == 'c') {
                fprintf(st_current_output, "%c", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 'd') {
                fprintf(st_current_output, "%d", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 'o') {
                fprintf(st_current_output, "%o", values[val_idx]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == 's') {
                put_str(st_current_output, (char*)&memory[values[val_idx]]);
                val_idx++;
            } else if (fmt[fmt_idx + 1] == '%') {
                fputc('%', st_current_output);
            }
            fmt_idx++;
        } else {
            fputc(fmt[fmt_idx], st_current_output);
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

    return 0;
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
fun_exit(int argc, int *args, int *memory)
{
    exit(0);
}

static char *st_builtin_function_names[] = {
    "printf",
    "putnumb",
    "putchar",
    "getchar",
    "exit",
};

BuiltinFunction
bvm_builtin_functions[] = {
    {"printf", fun_printf},
    {"putnumb", fun_putnumb},
    {"putchar", fun_putchar},
    {"getchar", fun_getchar},
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
