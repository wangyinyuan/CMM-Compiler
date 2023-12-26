#ifndef CMM_COMPILER_H
#define CMM_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../helpers/buffer.h"

// compile_process 成败返回值
enum
{
    SUCCESS,
    FAILURE
};

// lex_process 成败返回值
enum
{
    LEXICAL_ANALYSIS_ALL_OK,
    LEXICAL_ANALYSIS_ERROR
};

enum output_type
{
    OUTPUT_TYPE_ASSEMBLY,
    OUTPUT_TYPE_OBJECT,
    OUTPUT_TYPE_EXECUTABLE
};

#define NUMERIC_CASES \
    case '0':         \
    case '1':         \
    case '2':         \
    case '3':         \
    case '4':         \
    case '5':         \
    case '6':         \
    case '7':         \
    case '8':         \
    case '9'

// token types
enum
{
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_COMMENT,
    TOKEN_TYPE_NEWLINE,
    TOKEN_TYPE_SYMBOL,
};

typedef struct pos
{
    int line;
    int col;
    const char *filename;
} pos;

typedef struct token
{
    int type;
    int flags;
    pos pos;

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
        void *any;
    };

    /**
     * @brief 表示是否包含空白字符的布尔值。
     */
    bool whitespace;

    const char *between_brackets;
} token;

/**
 * @brief Represents the compile process information.
 *
 * This struct contains the necessary information for the compile process,
 * including the filename, output filename,
 * and output type.
 */
typedef struct compile_process compile_process;

typedef struct compile_process_input_file
{
    FILE *file;
    const char *abs_path;
} cfile;

struct compile_process
{

    cfile *input_file; ///< The input file.
    FILE *ofile;       ///< The output file.
    int output_type;   ///< The type of output for the compiler.

    pos pos; ///< The current position of the compiler.
};

// 词法分析器结构体定义

typedef struct lex_process_functions lex_process_functions;
typedef struct lex_process lex_process;
struct lex_process
{
    pos pos;
    struct vector *token_vec;
    compile_process *compiler;

    int current_expression_count;
    struct buffer *parentheses_buffer;
    lex_process_functions *function;

    void *private;
};

typedef char (*LEX_PROCESS_NEXT_CHAR)(lex_process *process);
typedef char (*LEX_PROCESS_PEEK_CHAR)(lex_process *process);
typedef void (*LEX_PROCESS_PUSH_CHAR)(lex_process *process, char c);

struct lex_process_functions
{
    LEX_PROCESS_NEXT_CHAR next_char;
    LEX_PROCESS_PEEK_CHAR peek_char;
    LEX_PROCESS_PUSH_CHAR push_char;
};

int compile_file(const char *filename, const char *output_filename, int output_type);
compile_process *compile_process_create(const char *filename, const char *output_filename, int output_type);

// lex_process_functions
char compile_process_next_char(lex_process *process);
char compile_process_peek_char(lex_process *process);
void compile_process_push_char(lex_process *process, char c);

// lex_process
lex_process *lex_process_create(compile_process *compiler, lex_process_functions *functions, void *data);
void lex_process_free(lex_process *process);
void *lex_process_private(lex_process *process);
struct vector *lex_process_tokens(lex_process *process);

// lexer
int lex(lex_process *process);

// error and warning
void compiler_error(compile_process *compiler, const char *msg, ...);
void compiler_warning(compile_process *compiler, const char *msg, ...);

#endif // CMM_COMPILER_H
