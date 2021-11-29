#include <stdio.h>
#include <stdlib.h>

void printMas(const int *mas, int size);
void selectionSort(int *l, int *r);

int main()
{
    int n = 0;
	
    printf("Input array size: ");
    scanf("%d", &n);

    if (n <= 0)
    {
        printf("Wrong size!");
        return -1;
    }

    int mas[n];

    printf("Input array: ");

    for (int i = 0; i < n; i++)
        scanf("%d", &mas[i]);

    printf("Inputed array = ");
    printMas(mas, n);

    selectionSort(&mas[0], &mas[n-1]);
    printf("Sorted array = ");
    printMas(mas, n);

    return 0;
}

void printMas(const int *mas, int size)
{
    for (int i = 0; i < size; i++)
    {
        printf("%d ", mas[i]);
    }
    printf("\n");
}

void swap(int *el1, int *el2)
{
    int temp = *el1;
    *el1 = *el2;
    *el2 = temp;
}

void selectionSort(int *l, int *r)
{
    for (int *i = l; i <= r; i++)
    {
        int minz = *i, *ind = i;
        for (int *j = i + 1; j <= r; j++)
        {
            if (*j < minz)
            {
                minz = *j;
                ind = j;
            }
        }
        swap(i, ind);
    }
}
