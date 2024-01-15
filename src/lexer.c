#include "compiler.h"
#include "token.h"
#include "../helpers/vector.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>

#define LEX_GETC_IF(buffer, c, exp)     \
    for (c = peekc(); exp; c = peekc()) \
    {                                   \
        buffer_write(buffer, c);        \
        nextc();                        \
    }

static lex_process *lex_process_instance;
static token tem_token;
token *read_next_token();
bool lex_is_in_expression();

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
    if (lex_is_in_expression())
    {
        buffer_write(lex_process_instance->parentheses_buffer, c);
    }
    lex_process_instance->pos.col++;
    if (c == '\n')
    {
        lex_process_instance->pos.line++;
        lex_process_instance->pos.col = 1;
    }
    return c;
}

static char assert_next_char(char c)
{
    char next = nextc();
    assert(c == next);
    return next;
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
    if (lex_is_in_expression())
    {
        tem_token.between_brackets = buffer_ptr(lex_process_instance->parentheses_buffer);
    }
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

int lexer_number_type(char c)
{
    int res = NUMBER_TYPE_NORMAL;
    if (c == 'L')
    {
        res = NUMBER_TYPE_LONG;
    }
    else if (c == 'f')
    {
        res = NUMBER_TYPE_FLOAT;
    }
    return res;
}

token *token_make_value_for_number(unsigned long number)
{
    int number_type = lexer_number_type(peekc());
    if (number_type != NUMBER_TYPE_NORMAL)
    {
        nextc();
    }
    return token_create(&(token){
        .type = TOKEN_TYPE_NUMBER,
        .llnum = number,
        .num.type = number_type,
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
    return op == '(' || op == '[' || op == ',' || op == '.' || op == '*' || op == '?';
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

static void lex_end_expression()
{
    lex_process_instance->current_expression_count--;
    if (lex_process_instance->current_expression_count < 0)
    {
        compiler_error(lex_process_instance->compiler, "Unexpected ')'\n");
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

static token *token_make_symbol()
{
    char c = nextc();

    if (c == ')')
    {
        lex_end_expression();
    }

    token *token_instance = token_create(&(token){
        .type = TOKEN_TYPE_SYMBOL,
        .cval = c,
    });
    return token_instance;
}

bool keyword_is_datatype(const char *str)
{
    return S_EQ(str, "void") ||
           S_EQ(str, "char") ||
           S_EQ(str, "int") ||
           S_EQ(str, "short") ||
           S_EQ(str, "float") ||
           S_EQ(str, "double") ||
           S_EQ(str, "long") ||
           S_EQ(str, "struct") ||
           S_EQ(str, "union");
}

bool is_keyword(const char *keyword)
{
    return S_EQ(keyword, "if") ||
           S_EQ(keyword, "else") ||
           S_EQ(keyword, "while") ||
           S_EQ(keyword, "for") ||
           S_EQ(keyword, "do") ||
           S_EQ(keyword, "switch") ||
           S_EQ(keyword, "case") ||
           S_EQ(keyword, "default") ||
           S_EQ(keyword, "break") ||
           S_EQ(keyword, "continue") ||
           S_EQ(keyword, "return") ||
           S_EQ(keyword, "goto") ||
           S_EQ(keyword, "typedef") ||
           S_EQ(keyword, "struct") ||
           S_EQ(keyword, "union") ||
           S_EQ(keyword, "enum") ||
           S_EQ(keyword, "extern") ||
           S_EQ(keyword, "static") ||
           S_EQ(keyword, "const") ||
           S_EQ(keyword, "volatile") ||
           S_EQ(keyword, "register") ||
           S_EQ(keyword, "auto") ||
           S_EQ(keyword, "void") ||
           S_EQ(keyword, "char") ||
           S_EQ(keyword, "short") ||
           S_EQ(keyword, "int") ||
           S_EQ(keyword, "long") ||
           S_EQ(keyword, "float") ||
           S_EQ(keyword, "double") ||
           S_EQ(keyword, "signed") ||
           S_EQ(keyword, "unsigned") ||
           S_EQ(keyword, "sizeof") ||
           S_EQ(keyword, "typeof") ||
           S_EQ(keyword, "bool") ||
           S_EQ(keyword, "true") ||
           S_EQ(keyword, "false") ||
           S_EQ(keyword, "NULL");
}

// 生成标识符 token
static token *
token_make_identifier_or_keyword()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || (c >= '0' && c <= '9'));

    buffer_write(buffer, 0x00);

    if (is_keyword(buffer_ptr(buffer)))
    {
        return token_create(&(token){
            .type = TOKEN_TYPE_KEYWORD,
            .sval = buffer_ptr(buffer),
        });
    }

    return token_create(&(token){
        .type = TOKEN_TYPE_IDENTIFIER,
        .sval = buffer_ptr(buffer),
    });
}

token *read_special_token()
{
    char c = peekc();
    if (isalpha(c) || c == '_')
    {
        return token_make_identifier_or_keyword();
    }

    return NULL;
}

// 生成换行符 token
static token *token_make_newline()
{
    nextc();
    return token_create(&(token){
        .type = TOKEN_TYPE_NEWLINE,
    });
}

// 单行注释
static token *token_make_one_line_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    LEX_GETC_IF(buffer, c, c != '\n' && c != EOF);
    return token_create(&(token){
        .type = TOKEN_TYPE_COMMENT,
        .sval = buffer_ptr(buffer),
    });
}

// 多行注释
static token *token_make_multiline_comment()
{
    struct buffer *buffer = buffer_create();
    char c = 0;
    while (1)
    {
        LEX_GETC_IF(buffer, c, c != '*' && c != EOF);
        if (c == EOF)
        {
            compiler_error(lex_process_instance->compiler, "你没有关闭多行注释\n");
        }
        if (c == '*')
        {
            nextc();
            if (peekc() == '/')
            {
                nextc();
                break;
            }
        }
    }

    return token_create(&(token){
        .type = TOKEN_TYPE_COMMENT,
        .sval = buffer_ptr(buffer),
    });
}

static token *handle_comment()
{
    char c = peekc();
    if (c == '/')
    {
        nextc();
        if (peekc() == '/')
        {
            nextc();
            return token_make_one_line_comment();
        }
        else if (peekc() == '*')
        {
            nextc();
            return token_make_multiline_comment();
        }

        pushc('/');
        return token_make_operator_or_string();
    }
    return NULL;
}

char lex_get_escaped_char(char c)
{
    char escaped_char = 0;
    switch (c)
    {
    case 'n':
        escaped_char = '\n';
        break;
    case '\\':
        escaped_char = '\\';
        break;
    case 't':
        escaped_char = '\t';
        break;
    case '\'':
        escaped_char = '\'';
        break;
    }
    return escaped_char;
}

token *token_make_quote()
{
    assert_next_char('\'');
    char c = nextc();
    if (c == '\\')
    {
        c = nextc();
        c = lex_get_escaped_char(c);
    }

    if (nextc() != '\'')
    {
        compiler_error(lex_process_instance->compiler, "你没有正确关闭单引号\n");
    }
    return token_create(&(token){
        .type = TOKEN_TYPE_NUMBER,
        .cval = c,
    });
}

token *read_next_token()
{
    token *token_instance = NULL;
    char c = peekc();

    token_instance = handle_comment();
    if (token_instance)
    {
        return token_instance;
    }
    switch (c)
    {
    NUMERIC_CASES:
        token_instance = token_make_number();
        break;
    OPERATOR_CASES_EXCLUDING_DIVISION:
        token_instance = token_make_operator_or_string();
        break;
    SYMBOL_CASE:
        token_instance = token_make_symbol();
        break;
    case ' ':
    case '\t':
        token_instance = handle_whitespace();
        break;
    case '"': // 字符串
        token_instance = token_make_string('"', '"');
        break;
    case '\'': // 字符
        token_instance = token_make_quote();
        break;
    case '\n':
        token_instance = token_make_newline();
        break;
    case EOF:
        // 读到文件尾部
        break;

    default:
        token_instance = read_special_token();
        if (!token_instance)
        {
            compiler_error(lex_process_instance->compiler, "Unexpected token\n");
        }
    }

    return token_instance;
}

int lex(lex_process *process)
{
    process->current_expression_count = 0;
    process->parentheses_buffer = NULL;
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

char lexer_string_buffer_next_char(lex_process *process)
{
    struct buffer *buffer = lex_process_private(process);
    return buffer_read(buffer);
}

char lexer_string_buffer_peek_char(lex_process *process)
{
    struct buffer *buffer = lex_process_private(process);
    return buffer_peek(buffer);
}

void lexer_string_buffer_push_char(lex_process *process, char c)
{
    struct buffer *buffer = lex_process_private(process);
    buffer_write(buffer, c);
}

lex_process_functions lexer_string_buffer_functions = {
    .next_char = lexer_string_buffer_next_char,
    .peek_char = lexer_string_buffer_peek_char,
    .push_char = lexer_string_buffer_push_char,
};

lex_process *token_build_for_string(compile_process *compiler, const char *str)
{
    struct buffer *buffer = buffer_create();
    buffer_printf(buffer, str);
    lex_process *process = lex_process_create(compiler, &lexer_string_buffer_functions, buffer);
    if (!lex_process_instance)
        return NULL;

    if (lex(lex_process_instance) != LEXICAL_ANALYSIS_ALL_OK)
        return NULL;

    return lex_process_instance;
}