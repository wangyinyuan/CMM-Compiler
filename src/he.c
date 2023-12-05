#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int main(int argc, char *argv[])
{
    printf("Hello, world!\n");
    int a = 1, b = 2;
    printf("%d + %d = %d\n", a, b, add(a, b));
    return 0;
}
