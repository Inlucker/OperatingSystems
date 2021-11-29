#include <stdio.h>

#define LENGTH 100

int main()
{
    int i = 0;
    char str[LENGTH];

    printf("Input the message: ");
    scanf("%s", str);
    printf("Inputed message: %s\n", str);
    return 0;
}
