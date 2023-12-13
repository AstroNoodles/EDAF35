#include <stdio.h>

int main() {
    printf("Hello World! \n");

    int arr[5] = {1, 2, 3, 4, 5};
    printf("%d \n", arr[0]);
    arr[0] = 2;
    printf("%d \n", arr[0]);
    arr[7] = 233;
    //printf("%d \n", arr[7]);
}