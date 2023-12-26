#include "compiler.h"
#include "../helpers/vector.h"
#include <string.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

static lex_process *lex_process_instance;
static token tem_token;

static char peekc()
{
    return lex_process_instance->function->peek_char(lex_process_instance);
}
static void pushc(char c)
{
    lex_process_instance->function->push_char(lex_process_instance, c);
}
static char nextc()
{
    char c = lex_process_instance->function->next_char(lex_process_instance);
    lex_process_instance->pos.col++;
    if (c == '\n')
    {
        lex_process_instance->pos.line++;
        lex_process_instance->pos.col = 1;
    }
    return c;
}

static pos lex_file_position()
{
    return lex_process_instance->pos;
}

token *token_create(token *_token)
{
    memcpy(&tem_token, _token, sizeof(token));
    tem_token.pos = lex_file_position();
    return &tem_token;
}

const char *read_number_str()
{
    // const char *num = NULL;
    struct buffer *buffer = buffer_create();
    char c = peekc();
    LEX_GETC_IF(buffer, c, (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);
    return buffer_ptr(buffer);
}

unsigned long long read_number()
{
    const char *num_str = read_number_str();
    return strtoull(num_str, NULL, 10);
}

token *token_make_value_for_number(unsigned long number)
{
    return token_create(&(token){
        .type = TOKEN_TYPE_NUMBER,
        .llnum = number,
    });
}

token *token_make_number()
{
    return token_make_value_for_number(read_number());
}

token *read_next_token()
{
    token *token_instance = NULL;
    char c = peekc();
    switch (c)
    {
    NUMERIC_CASES:
        token_instance = token_make_number();
        break;
    case EOF:
        // 读到文件尾部
        break;

    default:
        compiler_error(lex_process_instance->compiler, "Unexpected token\n");
    }

    return token_instance;
}

int lex(lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = buffer_create();
    lex_process_instance = process;
    process->pos.filename = process->compiler->input_file->abs_path;

    token *token_instance = read_next_token();
    while (token_instance)
    {
        vector_push(process->token_vec, token_instance);
        token_instance = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}
