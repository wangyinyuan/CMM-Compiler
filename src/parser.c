#include "compiler.h"
#include "../helpers/vector.h"
#include "token.h"
#include <assert.h>

struct history
{
    int flags;
};

extern struct node *parser_current_body;

int parse_expressionable_single(struct history *history);
void parse_expressionable(struct history *history);

extern struct expressionable_op_precedence_group op_precedence[TOTAL_OPERATOR_GROUPS];

static compile_process *current_process;
static struct token *parser_last_token;
static bool token_is_nl_or_comment_or_newline_seperator(struct token *token);
void parser_datatype_init_type_and_size_for_primitive(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out);
bool parser_is_int_valid_after_datatype(struct datatype *dtype);
void parse_ignore_int(struct datatype *dtype);

static bool token_is_nl_or_comment_or_newline_seperator(struct token *token)
{
    if (!token)
        return false;
    return token->type == TOKEN_TYPE_NEWLINE ||
           token->type == TOKEN_TYPE_COMMENT ||
           token_is_symbol(token, '\\');
}

// history

struct history *history_begin(int flags)
{
    struct history *history = (struct history *)malloc(sizeof(struct history));
    history->flags = flags;
    return history;
}

struct history *history_down(struct history *history, int flags)
{
    struct history *new_history = (struct history *)malloc(sizeof(struct history));
    memcpy(new_history, history, sizeof(struct history));
    new_history->flags = flags;
    return new_history;
}

static void parser_ignore_nl_or_comment(struct token *token)
{
    while (token && token_is_nl_or_comment_or_newline_seperator(token))
    {
        vector_peek(current_process->token_vec);
        token = vector_peek_no_increment(current_process->token_vec);
    }
}

void parser_scope_new()
{
    scope_new(current_process, 0);
}

void parser_scope_finish()
{
    scope_finish(current_process);
}

static struct token *token_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    current_process->pos = next_token->pos;
    parser_last_token = next_token;
    return vector_peek(current_process->token_vec);
}

static token *token_peek_next()
{
    struct token *next_token = vector_peek_no_increment(current_process->token_vec);
    parser_ignore_nl_or_comment(next_token);
    return vector_peek_no_increment(current_process->token_vec);
}

static void expect_sym(char c)
{
    struct token *next_token = token_next();
    if (!next_token || next_token->type != TOKEN_TYPE_SYMBOL || next_token->cval != c)
    {
        compiler_error(current_process, "Expected symbol %c", c);
    }
}

static void expect_op(const char *op)
{
    struct token *next_token = token_next();
    if (next_token == NULL || next_token->type != TOKEN_TYPE_OPERATOR || !S_EQ(next_token->sval, op))
        compiler_error(current_process, "Expecting the operator %s but something else was provided", op);
}

static bool token_next_is_operator(const char *op)
{
    struct token *token = token_peek_next();
    return token_is_operator(token, op);
}

static bool token_next_is_symbol(char sym)
{
    struct token *token = token_peek_next();
    return token_is_symbol(token, sym);
}

void parse_single_token_to_node()
{
    struct token *token = token_next();
    struct node *node = NULL;
    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        node = node_create(&(struct node){.type = NODE_TYPE_NUMBER, .llnum = token->llnum});
        break;

    case TOKEN_TYPE_IDENTIFIER:
        node = node_create(&(struct node){.type = NODE_TYPE_IDENTIFIER, .sval = token->sval});
        break;

    case TOKEN_TYPE_STRING:
        node = node_create(&(struct node){.type = NODE_TYPE_STRING, .sval = token->sval});
        break;

    default:
        compiler_error(current_process, "此 token 无法转换为 node");
    }
}

void parse_expressionable_for_op(struct history *history, const char *op)
{
    parse_expressionable(history);
}

static int parser_get_precedence_for_operator(const char *op, struct expressionable_op_precedence_group **group_out)
{
    *group_out = NULL;
    for (int i = 0; i < TOTAL_OPERATOR_GROUPS; i++)
    {
        for (int b = 0; op_precedence[i].operators[b]; b++)
        {
            const char *_op = op_precedence[i].operators[b];
            if (S_EQ(op, _op))
            {

                *group_out = &op_precedence[i];
                return i;
            }
        }
    }
}

static bool parser_left_op_has_priority(const char *op_left, const char *op_right)
{
    struct expressionable_op_precedence_group *group_left = NULL;
    struct expressionable_op_precedence_group *group_right = NULL;

    // Same operator? Then they have equal priority!
    if (S_EQ(op_left, op_right))
        return false;

    int precedence_left = parser_get_precedence_for_operator(op_left, &group_left);
    int precedence_right = parser_get_precedence_for_operator(op_right, &group_right);
    if (group_left->associativity == ASSOCIATIVITY_RIGHT_TO_LEFT)
    {
        return false;
    }

    return precedence_left <= precedence_right;
}

void parser_node_shift_children_left(struct node *node)
{
    assert(node->type == NODE_TYPE_EXPRESSION);
    assert(node->exp.right->type == NODE_TYPE_EXPRESSION);

    const char *right_op = node->exp.right->exp.op;
    struct node *new_exp_left_node = node->exp.left;
    struct node *new_exp_right_node = node->exp.right->exp.left;
    make_exp_node(new_exp_left_node, new_exp_right_node, node->exp.op);

    struct node *new_left_operand = node_pop();
    struct node *new_right_operand = node->exp.right->exp.right;
    node->exp.left = new_left_operand;
    node->exp.right = new_right_operand;
    node->exp.op = right_op;
}

void parser_reorder_expression(struct node **node_out)
{
    struct node *node = *node_out;
    if (node->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }
    // 无需重排
    if (node->exp.left->type != NODE_TYPE_EXPRESSION && node->exp.right && node->exp.right->type != NODE_TYPE_EXPRESSION)
    {
        return;
    }

    if (node->exp.left->type != NODE_TYPE_EXPRESSION && node->exp.right && node->exp.right->type == NODE_TYPE_EXPRESSION)
    {
        const char *op = node->exp.right->exp.op;
        if (parser_left_op_has_priority(node->exp.op, op))
        {
            parser_node_shift_children_left(node);
            parser_reorder_expression(&node->exp.left);
            parser_reorder_expression(&node->exp.right);
        }
        return;
    }
}

void parse_expression_normal(struct history *history)
{
    struct token *token = token_peek_next();
    const char *op = token->sval;
    struct node *node_left = node_peek_expressionable_or_null();
    if (!node_left)
    {
        return;
    }

    token_next();

    node_pop();

    node_left->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    parse_expressionable_for_op(history_down(history, history->flags), op);
    struct node *node_right = node_pop();
    node_right->flags |= NODE_FLAG_INSIDE_EXPRESSION;

    make_exp_node(node_left, node_right, op);
    struct node *exp_node = node_pop();

    // 记录表达式
    parser_reorder_expression(&exp_node);

    node_push(exp_node);
}

int parse_exp(struct history *history)
{
    parse_expression_normal(history);
    return 0;
}

void parse_identifier(struct history *history)
{
    assert(token_peek_next()->type == NODE_TYPE_IDENTIFIER);
    parse_single_token_to_node();
}

static bool is_keyword_variable_modifier(const char *val)
{
    return S_EQ(val, "unsigned") ||
           S_EQ(val, "signed") ||
           S_EQ(val, "static") ||
           S_EQ(val, "const") ||
           S_EQ(val, "extern") ||
           S_EQ(val, "__ignore_typecheck__");
}

void parse_datatype_modifiers(struct datatype *dtype)
{
    struct token *token = token_peek_next();
    while (token && token->type == TOKEN_TYPE_KEYWORD)
    {
        if (!is_keyword_variable_modifier(token->sval))
        {
            break;
        }

        if (S_EQ(token->sval, "signed"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "unsigned"))
        {
            dtype->flags &= ~DATATYPE_FLAG_IS_SIGNED;
        }
        else if (S_EQ(token->sval, "static"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_STATIC;
        }
        else if (S_EQ(token->sval, "const"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_CONST;
        }
        else if (S_EQ(token->sval, "extern"))
        {
            dtype->flags |= DATATYPE_FLAG_IS_EXTERN;
        }
        else if (S_EQ(token->sval, "__ignore_typecheck__"))
        {
            dtype->flags |= DATATYPE_FLAG_IGNORE_TYPE_CHECKING;
        }

        token_next();
        token = token_peek_next();
    }
}

void parser_get_datatype_tokens(struct token **datatype_token, struct token **datatype_secondary_token)
{
    *datatype_token = token_next();
    struct token *next_token = token_peek_next();
    if (token_is_primitive_keyword(next_token))
    {
        *datatype_secondary_token = next_token;
        token_next();
    }
}

int parser_datatype_expected_for_type_string(const char *str)
{
    int type = DATA_TYPE_EXPECT_PRIMITIVE;
    if (S_EQ(str, "union"))
    {
        type = DATA_TYPE_EXPECT_UNION;
    }
    else if (S_EQ(str, "struct"))
    {
        type = DATA_TYPE_EXPECT_STRUCT;
    }

    return type;
}

token *parser_build_random_type_name()
{
    char tmp_name[32];
    sprintf(tmp_name, "__%d", rand());
    char *sval = malloc(sizeof(tmp_name));
    strncpy(sval, tmp_name, sizeof(tmp_name));
    token *token = calloc(1, sizeof(token));
    token->type = TOKEN_TYPE_IDENTIFIER;
    token->sval = sval;
    return token;
}

int parser_get_pointer_depth()
{
    int depth = 0;
    while (token_next_is_operator("*"))
    {
        depth++;
        token_next();
    }
    return depth;
}

bool parser_datatype_is_secondary_allowed(int expected_type)
{
    return expected_type == DATA_TYPE_EXPECT_PRIMITIVE;
}

bool parser_datatype_is_secondary_allowed_for_type(const char *type)
{
    return S_EQ(type, "long") || S_EQ(type, "short") || S_EQ(type, "double") || S_EQ(type, "float");
}

void parser_datatype_adjust_size_for_secondary(struct datatype *datatype, struct token *datatype_secondary_token)
{
    if (!datatype_secondary_token)
    {
        return;
    }

    struct datatype *secondary_data_type = calloc(sizeof(struct datatype), 1);
    parser_datatype_init_type_and_size_for_primitive(datatype_secondary_token, NULL, secondary_data_type);
    datatype->size += secondary_data_type->size;
    datatype->secondary = secondary_data_type;
    datatype->flags |= DATATYPE_FLAG_SECONDARY;
}

void parser_datatype_init_type_and_size_for_primitive(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out)
{
    if (!parser_datatype_is_secondary_allowed_for_type(datatype_token->sval) && datatype_secondary_token)
    {
        // no secondary is allowed
        compiler_error(current_process, "Your not allowed a secondary datatype here for the given datatype %s", datatype_token->sval);
    }

    if (S_EQ(datatype_token->sval, "void"))
    {
        datatype_out->type = DATA_TYPE_VOID;
        datatype_out->size = 0;
    }
    else if (S_EQ(datatype_token->sval, "char"))
    {
        datatype_out->type = DATA_TYPE_CHAR;
        datatype_out->size = 1;
    }
    else if (S_EQ(datatype_token->sval, "short"))
    {
        datatype_out->type = DATA_TYPE_SHORT;
        datatype_out->size = 2;
    }
    else if (S_EQ(datatype_token->sval, "int"))
    {
        datatype_out->type = DATA_TYPE_INTEGER;
        datatype_out->size = 4;
    }
    else if (S_EQ(datatype_token->sval, "long"))
    {
        // We are a 32 bit compiler so long is 4 bytes.
        datatype_out->type = DATA_TYPE_LONG;
        datatype_out->size = 4;
    }
    else if (S_EQ(datatype_token->sval, "float"))
    {
        datatype_out->type = DATA_TYPE_FLOAT;
        datatype_out->size = 4;
    }
    else if (S_EQ(datatype_token->sval, "double"))
    {
        datatype_out->type = DATA_TYPE_DOUBLE;
        datatype_out->size = 4;
    }
    else
    {
        compiler_error(current_process, "Bug unexpected primitive variable\n");
    }

    parser_datatype_adjust_size_for_secondary(datatype_out, datatype_secondary_token);
}

void parser_datatype_init_type_and_size(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out, int pointer_depth, int expected_type)
{

    if (!parser_datatype_is_secondary_allowed(expected_type) && datatype_secondary_token)
    {
        compiler_error(current_process, "You provided an extra datatype yet this is not a primitive variable");
    }

    switch (expected_type)
    {
    case DATA_TYPE_EXPECT_PRIMITIVE:
        parser_datatype_init_type_and_size_for_primitive(datatype_token, datatype_secondary_token, datatype_out);
        break;

    case DATA_TYPE_EXPECT_UNION:
    case DATA_TYPE_EXPECT_STRUCT:
        break;

    default:
        compiler_error(current_process, "Compiler bug unexpected data type expectation");
    }

    if (pointer_depth > 0)
    {
        datatype_out->flags |= DATATYPE_FLAG_IS_POINTER;
        datatype_out->pointer_depth = pointer_depth;
    }
}

void parser_datatype_init(struct token *datatype_token, struct token *datatype_secondary_token, struct datatype *datatype_out, int pointer_depth, int expected_type)
{
    parser_datatype_init_type_and_size(datatype_token, datatype_secondary_token, datatype_out, pointer_depth, expected_type);
    datatype_out->type_str = datatype_token->sval;

    if (S_EQ(datatype_token->sval, "long") && datatype_secondary_token && S_EQ(datatype_secondary_token->sval, "long"))
    {
        compiler_warning(current_process, "Our compiler does not support 64 bit long long so it will be treated as a 32 bit type not 64 bit\n");
        datatype_out->size = DATA_SIZE_DWORD;
    }
}

void parse_datatype_type(struct datatype *dtype)
{
    token *datatype_token = NULL;
    token *datatype_secondary_token = NULL;
    parser_get_datatype_tokens(&datatype_token, &datatype_secondary_token);
    int expected_type = parser_datatype_expected_for_type_string(datatype_token->sval);
    if (datatype_is_struct_or_union_for_name(datatype_token->sval))
    {
        if (token_peek_next()->type == TOKEN_TYPE_IDENTIFIER)
        {
            datatype_token = token_next();
        }
        else
        {
            // 匿名结构体或联合体
            datatype_token = parser_build_random_type_name();
            dtype->flags |= DATATYPE_FLAG_STRUCT_UNION_NO_NAME;
        }
    }

    int pointer_depth = parser_get_pointer_depth();
    parser_datatype_init(datatype_token, datatype_secondary_token, dtype, pointer_depth, expected_type);
}
void parse_datatype(struct datatype *dtype)
{
    memset(dtype, 0, sizeof(struct datatype));
    dtype->flags != DATATYPE_FLAG_IS_SIGNED;

    parse_datatype_modifiers(dtype);
    parse_datatype_type(dtype);
    parse_datatype_modifiers(dtype);
}

void parse_expressionable_root(struct history *history)
{
    parse_expressionable(history);
    struct node *result_node = node_pop();
    node_push(result_node);
}

void make_variable_node(struct datatype *dtype, struct token *name_token, struct node *value_node)
{
    const char *name_str = NULL;
    if (name_token)
    {
        name_str = name_token->sval;
    }

    node_create(&(struct node){.type = NODE_TYPE_VARIABLE, .var.type = *dtype, .var.name = name_str, .var.val = value_node});
}

void make_variable_node_and_register(struct history *history, struct datatype *dtype, struct token *name_token, struct node *value_node)
{
    make_variable_node(dtype, name_token, value_node);
    struct node *var_node = node_pop();

    // parser_scope_offset(var_node, history);
    // parser_scope_push(parser_new_scope_entity(var_node, var_node->var.aoffset, 0), var_node->var.type.size);
    // resolver_default_new_scope_entity(current_process->resolver, var_node, var_node->var.aoffset, 0);

    node_push(var_node);
}

void make_variable_list_node(struct vector *var_list_vec)
{
    node_create(&(struct node){.type = NODE_TYPE_VARIABLE_LIST, .var_list.list = var_list_vec});
}

void parse_variable(struct datatype *dtype, struct token *name_token, struct history *history)
{
    struct node *value_node = NULL;
    if (token_next_is_operator("="))
    {
        token_next();
        parse_expressionable_root(history);
        value_node = node_pop();
    }

    make_variable_node_and_register(history, dtype, name_token, value_node);
}

void parse_variable_function_or_struct_union(struct history *history)
{
    struct datatype dtype;
    parse_datatype(&dtype);

    // TODO: 处理 struct and union

    parse_ignore_int(&dtype);

    struct token *name_token = token_next();
    if (name_token->type != TOKEN_TYPE_IDENTIFIER)
    {
        compiler_error(current_process, "Expected identifier after datatype");
    }

    parse_variable(&dtype, name_token, history);
    if (token_is_operator(token_peek_next(), ","))
    {
        struct vector *var_list = vector_create(sizeof(struct node *));
        struct node *var_node = node_pop();
        vector_push(var_list, &var_node);
        while (token_is_operator(token_peek_next(), ","))
        {
            token_next();
            name_token = token_next();
            parse_variable(&dtype, name_token, history);
            var_node = node_pop();
            vector_push(var_list, &var_node);
        }
        make_variable_list_node(var_list);
    }
    expect_sym(';');
}

bool parser_is_int_valid_after_datatype(struct datatype *dtype)
{
    return dtype->type == DATA_TYPE_FLOAT ||
           dtype->type == DATA_TYPE_DOUBLE ||
           dtype->type == DATA_TYPE_LONG;
}

void parse_ignore_int(struct datatype *dtype)
{
    if (!token_is_keyword(token_peek_next(), "int"))
    {
        return;
    }
    if (!parser_is_int_valid_after_datatype(dtype))
    {
        compiler_error(current_process, "You cannot use int after this datatype");
    }

    token_next();
}

void parse_keyword(struct history *history)
{
    struct token *token = token_peek_next();
    if (is_keyword_variable_modifier(token->sval) || keyword_is_datatype(token->sval))
    {
        parse_variable_function_or_struct_union(history);
        return;
    }
}

int parse_expressionable_single(struct history *history)
{
    struct token *token = token_peek_next();
    if (!token)
    {
        return -1;
    }

    history->flags |= NODE_FLAG_INSIDE_EXPRESSION;
    int res = -1;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
        parse_single_token_to_node();
        res = 0;
        break;
    case TOKEN_TYPE_IDENTIFIER:
        parse_identifier(history);
        res = 0;
        break;
    case TOKEN_TYPE_OPERATOR:
        parse_exp(history);
        res = 0;
        break;
    case TOKEN_TYPE_KEYWORD:
        parse_keyword(history);
        res = 0;
        break;
    }
    return res;
}

void parse_expressionable(struct history *history)
{
    while (parse_expressionable_single(history) == 0)
    {
    }
}

void parse_symbol()
{
    compiler_error(current_process, "symbol not implemented");
}

void parse_statement(struct history *history)
{
    if (token_peek_next()->type == TOKEN_TYPE_KEYWORD)
    {
        parse_keyword(history);
        return;
    }
    parse_expressionable_root(history);
    if (token_peek_next()->type == TOKEN_TYPE_SYMBOL && !token_is_symbol(token_peek_next(), ';'))
    {
        parse_symbol();
        return;
    }

    expect_sym(';');
}

void parser_append_size_for_node(struct history *history, size_t *_variable_size, struct node *node)
{
    compiler_warning(current_process, "parser_append_size_for_node not implemented");
}

void parser_finalize_body(struct history *history, struct node *body_node, struct vector *body_vec, size_t *_variable_size, struct node *largest_align_eligible_var_node, struct node *largest_possible_var_node)
{
    //     if (history->flags & HISTORY_FLAG_INSIDE_UNION)
    //     {
    //         // Unions variable size is equal to the largest variable node size
    //         if (largest_possible_var_node)
    //         {
    //             *_variable_size = variable_size(largest_possible_var_node);
    //         }
    //     }

    //     // Variable size should be adjusted to + the padding of all the body variables padding
    //     int padding = compute_sum_padding(body_vec);
    //     *_variable_size += padding;

    //     // Our own variable size must pad to the largest member
    //     if (largest_align_eligible_var_node)
    //     {
    //         *_variable_size = align_value(*_variable_size, largest_align_eligible_var_node->var.type.size);
    //     }
    //     // Let's make the body node now we have parsed all statements.
    // bool padded = padding != 0;
    body_node->body.largest_var_node = largest_align_eligible_var_node;
    // body_node->body.padded = padded;
    body_node->body.size = *_variable_size;
    body_node->body.statements = body_vec;
}

void parse_body_single_statement(size_t *variable_size, struct vector *body_vec, struct history *history)
{
    make_body_node(NULL, 0, false, NULL);
    struct node *body_node = node_pop();
    body_node->binded.owner = parser_current_body;
    parser_current_body = body_node;
    struct node *stmt_node = NULL;
    parse_statement(history_down(history, history->flags));
    stmt_node = node_pop();
    vector_push(body_vec, &stmt_node);

    parser_append_size_for_node(history, variable_size, stmt_node);
    struct node *largest_var_node = NULL;
    if (stmt_node->type == NODE_TYPE_VARIABLE)
    {
        largest_var_node = stmt_node;
    }

    parser_finalize_body(history, body_node, body_vec, variable_size, largest_var_node, largest_var_node);
    parser_current_body = body_node->binded.owner;

    node_push(body_node);
}

void parse_body(size_t *variable_size, struct history *history)
{
    parser_scope_new();
    size_t tem_size = 0x00;
    if (!variable_size)
    {
        variable_size = &tem_size;
    }
    struct vector *body_vec = vector_create(sizeof(struct node *));
    if (!token_next_is_symbol('{'))
    {
        parse_body_single_statement(variable_size, body_vec, history);
    }
    parser_scope_finish();
}

void parse_keyword_for_global()
{
    parse_keyword(history_begin(0));
    struct node *node = node_pop();

    node_push(node);
}

int parse_next()
{
    token *token = token_peek_next();

    if (!token)
    {
        return -1;
    }

    int res = 0;

    switch (token->type)
    {
    case TOKEN_TYPE_NUMBER:
    case TOKEN_TYPE_STRING:
    case TOKEN_TYPE_IDENTIFIER:
        parse_expressionable(history_begin(0));
        break;
    case TOKEN_TYPE_KEYWORD:
        parse_keyword_for_global();
        break;
    default:
        break;
    }

    return 0;
}

int parse(compile_process *compiler)
{
    scope_create_root(compiler);
    current_process = compiler;
    parser_last_token = NULL;
    node_set_vector(compiler->node_vec, compiler->node_tree_vec);
    struct node *node = NULL;

    vector_set_peek_pointer(compiler->token_vec, 0);

    while (parse_next() == 0)
    {
        node = node_peek();
        vector_push(compiler->node_tree_vec, &node);
    }
    return PARSE_ALL_OK;
}