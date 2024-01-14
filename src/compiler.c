#include "compiler.h"
#include <stdarg.h>

lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

void compiler_error(compile_process *compiler, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, "%s:%d:%d: error: ", compiler->input_file->abs_path, compiler->pos.line, compiler->pos.col);
    exit(-1);
}

void compiler_warning(compile_process *compiler, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    fprintf(stderr, "%s:%d:%d: warning: ", compiler->input_file->abs_path, compiler->pos.line, compiler->pos.col);
}

// 编译器主入口
int compile_file(const char *filename, const char *output_filename, int output_type)
{
    compile_process *process = compile_process_create(filename, output_filename, output_type);
    if (!process)
        return FAILURE;

    // lexical analysis

    lex_process *lex_process_instance = lex_process_create(process, &compiler_lex_functions, NULL);

    if (!lex_process_instance)
        return FAILURE;
    if (lex(lex_process_instance) != LEXICAL_ANALYSIS_ALL_OK)
        return FAILURE;

    process->token_vec = lex_process_instance->token_vec;

    // parsing

    if (parse(process) != PARSE_ALL_OK)
    {
        return FAILURE;
    }

    return SUCCESS;
}