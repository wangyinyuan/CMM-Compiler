#include "token.h"

bool token_is_keyword(token *token_instance, const char *keyword)
{
    return token_instance && token_instance->type == TOKEN_TYPE_KEYWORD && S_EQ(token_instance->sval, keyword);
}

bool token_is_symbol(struct token *token, char sym)
{
    return token && token->type == TOKEN_TYPE_SYMBOL && token->cval == sym;
}
