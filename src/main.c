#include <stdio.h>
#include "../helpers/vector.h"
#include "compiler.h"

int main()
{
    int res = compile_file("tests/test.cmm", "tests/test.s", OUTPUT_TYPE_ASSEMBLY);
    if (res == FAILURE)
        printf("Compilation failed.\n");
    else
        printf("Compilation succeeded.\n");
    return 0;
}