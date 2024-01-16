#include "compiler.h"

bool datatype_is_struct_or_union_for_name(const char *name)
{
    return S_EQ(name, "struct") || S_EQ(name, "union");
}

size_t datatype_element_size(struct datatype *dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_POINTER)
    {
        return DATA_SIZE_DWORD;
    }

    return dtype->size;
}

size_t datatype_size_no_ptr(struct datatype *dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_ARRAY)
    {
        return dtype->array.size;
    }

    return dtype->size;
}

size_t datatype_size(struct datatype *dtype)
{
    if (dtype->flags & DATATYPE_FLAG_IS_POINTER && dtype->pointer_depth > 0)
    {
        return DATA_SIZE_DWORD;
    }
}