#include "compiler.h"
#include "token.h"
#include "../helpers/vector.h"
#include <string.h>
#include <assert.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

static lex_process *lex_process_instance;
static token tem_token;
token *read_next_token();

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

static token *lexer_last_token()
{
    return vector_back_or_null(lex_process_instance->token_vec);
}

// 处理空白字符
static token *handle_whitespace()
{
    token *last_token = lexer_last_token();
    if (last_token)
    {
        last_token->whitespace = true;
    }
    nextc();
    return read_next_token();
}

token *
token_create(token *_token)
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

// 生成字符 token
static token *token_make_string(char start_char, char end_char)
{
    struct buffer *buffer = buffer_create();
    assert(nextc() == start_char);
    char c = nextc();
    for (; c != end_char && c != EOF; c = nextc())
    {
        if (c == '\\')
        {
            continue;
        }
        buffer_write(buffer, c);
    }
    buffer_write(buffer, 0x00);
    return token_create(&(token){
        .type = TOKEN_TYPE_STRING,
        .sval = buffer_ptr(buffer),
    });
}

static bool op_treated_as_one(char op)
{
    return op == '(' || op == '[' || op == '.' || op == ',' || op == '?' || op == '*' || op == ':';
}

static bool is_single_operator(char op)
{
    return op == '+' || op == '-' || op == '*' || op == '/' || op == '%' || op == '=' || op == '!' || op == '>' || op == '<' || op == '&' || op == '|' || op == '^' || op == '?' || op == '(' || op == '[' || op == '.' || op == ',';
}

bool op_valid(const char *op)
{
    return S_EQ(op, "+") ||
           S_EQ(op, "-") ||
           S_EQ(op, "*") ||
           S_EQ(op, "/") ||
           S_EQ(op, "!") ||
           S_EQ(op, "^") ||
           S_EQ(op, "+=") ||
           S_EQ(op, "-=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, ">>=") ||
           S_EQ(op, "<<=") ||
           S_EQ(op, "/=") ||
           S_EQ(op, ">>") ||
           S_EQ(op, "<<") ||
           S_EQ(op, ">=") ||
           S_EQ(op, "<=") ||
           S_EQ(op, ">") ||
           S_EQ(op, "<") ||
           S_EQ(op, "||") ||
           S_EQ(op, "&&") ||
           S_EQ(op, "|") ||
           S_EQ(op, "&") ||
           S_EQ(op, "++") ||
           S_EQ(op, "--") ||
           S_EQ(op, "=") ||
           S_EQ(op, "*=") ||
           S_EQ(op, "^=") ||
           S_EQ(op, "==") ||
           S_EQ(op, "!=") ||
           S_EQ(op, "->") ||
           S_EQ(op, "**") ||
           S_EQ(op, "(") ||
           S_EQ(op, "[") ||
           S_EQ(op, ",") ||
           S_EQ(op, ".") ||
           S_EQ(op, "...") ||
           S_EQ(op, "~") ||
           S_EQ(op, "?") ||
           S_EQ(op, "%");
}

void read_op_flush_back_keep_first(struct buffer *buffer)
{
    const char *data = buffer_ptr(buffer);
    int len = buffer->len;
    for (int i = len - 1; i >= 1; i--)
    {
        if (data[i] == 0x00)
        {
            continue;
        }

        pushc(data[i]);
    }
}

const char *read_op()
{
    bool single_operator = true;
    char op = nextc();
    struct buffer *buffer = buffer_create();
    buffer_write(buffer, op);

    if (op_treated_as_one(op))
    {
        op = peekc();
        if (is_single_operator(op))
        {
            buffer_write(buffer, op);
            nextc();
            single_operator = false;
        }
    }
    buffer_write(buffer, 0x00);
    char *ptr = buffer_ptr(buffer);
    if (!single_operator)
    {
        if (!op_valid(ptr))
        {
            read_op_flush_back_keep_first(buffer);
            ptr[1] = 0x00;
        }
    }

    else if (!op_valid(ptr))
    {
        compiler_error(lex_process_instance->compiler, "Unexpected operator %s\n", ptr);
    }

    return ptr;
}

static void lex_new_expression()
{
    lex_process_instance->current_expression_count++;
    if (lex_process_instance->current_expression_count == 1)
    {
        lex_process_instance->parentheses_buffer = buffer_create();
    }
}

bool lex_is_in_expression()
{
    return lex_process_instance->current_expression_count > 0;
}

// 生成操作符 token
static token *token_make_operator_or_string()
{
    char op = peekc();
    if (op == '<')
    {
        token *last_token = lexer_last_token();
        if (token_is_keyword(last_token, "include"))
        {
            return token_make_string('<', '>');
        }
    }

    token *token_instance = token_create(&(token){
        .type = TOKEN_TYPE_OPERATOR,
        .sval = read_op(),
    });

    if (op == '(')
    {
        lex_new_expression();
    }

    return token_instance;
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
    OPERATOR_CASES_EXCLUDE_DIVISION:
        token_instance = token_make_operator_or_string();
        break;
    case ' ':
    case '\t':
        token_instance = handle_whitespace();
        break;
    case '"': // 字符串
        token_instance = token_make_string('"', '"');
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
        // if (token_instance->type == TOKEN_TYPE_NUMBER)
        // {
        //     printf("%llu", token_instance->llnum);
        // }

        vector_push(process->token_vec, token_instance);
        token_instance = read_next_token();
    }

    return LEXICAL_ANALYSIS_ALL_OK;
}
