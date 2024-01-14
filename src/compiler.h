#ifndef CMM_COMPILER_H
#define CMM_COMPILER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../helpers/buffer.h"

#define S_EQ(str1, str2) \
    (str1 && str2 && strcmp(str1, str2) == 0)

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

// 操作符
#define OPERATOR_CASES_EXCLUDING_DIVISION \
    case '+':                             \
    case '-':                             \
    case '*':                             \
    case '>':                             \
    case '<':                             \
    case '^':                             \
    case '%':                             \
    case '!':                             \
    case '=':                             \
    case '~':                             \
    case '|':                             \
    case '&':                             \
    case '(':                             \
    case '[':                             \
    case ',':                             \
    case '.':                             \
    case '?'

#define SYMBOL_CASE \
    case '{':       \
    case '}':       \
    case ':':       \
    case ';':       \
    case '#':       \
    case '\\':      \
    case ')':       \
    case ']'

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

enum
{
    NUMBER_TYPE_NORMAL,
    NUMBER_TYPE_LONG,
    NUMBER_TYPE_FLOAT,
    NUMBER_TYPE_DOUBLE
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

    struct token_number
    {
        int type;
    } num;

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

    /**
     * @brief 编译器结构体
     *
     * 该结构体包含了编译器所需的向量数据结构。
     */
    struct vector *token_vec;     /**< 词法分析结果向量 */
    struct vector *node_vec;      /**< 语法分析结果向量 */
    struct vector *node_tree_vec; /**< 语法树向量 */
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

enum
{
    PARSE_ALL_OK,
    PARSE_GENERAL_ERROR
};

enum
{
    NODE_TYPE_EXPRESSION,
    NODE_TYPE_EXPRESSION_PARENTHESIS,
    NODE_TYPE_NUMBER,
    NODE_TYPE_IDENTIFIER,
    NODE_TYPE_STRING,
    NODE_TYPE_VARIABLE,
    NODE_TYPE_VARIABLE_LIST,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_BODY,
    NODE_TYPE_STATEMENT_RETURN,
    NODE_TYPE_STATEMENT_IF,
    NODE_TYPE_STATEMENT_ELSE,
    NODE_TYPE_STATEMENT_WHILE,
    NODE_TYPE_STATEMENT_DO_WHILE,
    NODE_TYPE_STATEMENT_FOR,
    NODE_TYPE_STATEMENT_BREAK,
    NODE_TYPE_STATEMENT_CONTINUE,
    NODE_TYPE_STATEMENT_SWITCH,
    NODE_TYPE_STATEMENT_CASE,
    NODE_TYPE_STATEMENT_DEFAULT,
    NODE_TYPE_STATEMENT_GOTO,
    NODE_TYPE_TENARY,
    NODE_TYPE_LABEL,
    NODE_TYPE_UNARY,
    NODE_TYPE_STRUCT,
    NODE_TYPE_UNION,
    NODE_TYPE_BRACKET,
    NODE_TYPE_CAST,
    NODE_TYPE_BLANK
};

struct node
{
    int type;
    int flags;

    struct pos pos;

    struct node_binded
    {
        struct node *owner;
        struct node *function;
    } binded;

    union
    {
        struct exp
        {
            struct node *left;
            struct node *right;
            const char *op;
        } exp;
    };

    union
    {
        char cval;
        const char *sval;
        unsigned int inum;
        unsigned long lnum;
        unsigned long long llnum;
    };
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

lex_process *token_build_for_string(compile_process *compiler, const char *str);

// parser
int parse(compile_process *compiler);

// node

void node_set_vector(struct vector *vec, struct vector *root_vec);
void node_push(struct node *node);
struct node *node_peek_or_null();
struct node *node_peek();
struct node *node_pop();
struct node *node_create(struct node *node);
bool node_is_expressionable(struct node *node);
struct node *node_peek_expressionable_or_null();
void make_exp_node(struct node *left_node, struct node *right_node, const char *op);

// history
enum
{
    NODE_FLAG_INSIDE_EXPRESSION = 0b00000001,
    NODE_FLAG_CLONED = 0b00000010,
    NODE_FLAG_IS_FORWARD_DECLARATION = 0b00000100,
    NODE_FLAG_HAS_VARIABLE_COMBINED = 0b00001000
};

#endif // CMM_COMPILER_H
