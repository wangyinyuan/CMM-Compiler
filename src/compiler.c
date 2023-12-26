#include "compiler.h"

lex_process_functions compiler_lex_functions = {
    .next_char = compile_process_next_char,
    .peek_char = compile_process_peek_char,
    .push_char = compile_process_push_char,
};

// 编译器主入口
int compile_file(const char *filename, const char *output_filename, int output_type)
{
    compile_process *process = compile_process_create(filename, output_filename, output_type);
    if (!process)
        return FAILURE;

    // lexical analysis
    lex_process *lex_process = lex_process_create(process, &compiler_lex_functions, NULL);
    if (!lex_process)
        return FAILURE;
    if (lex(lex_process) != LEXICAL_ANALYSIS_ALL_OK)
        return FAILURE;

    // parsing

    return SUCCESS;
}