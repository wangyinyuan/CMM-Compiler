#include "token.h"

bool token_is_keyword(token *token_instance, const char *keyword)
{
    return token_instance->type == TOKEN_TYPE_KEYWORD && S_EQ(token_instance->sval, keyword);
}
