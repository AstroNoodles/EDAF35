#include <stdio.h>

int main(int argc, char* vargs[]) {
 printf("%d \n", argc);

for(int i = 0; i < argc; i++){
    printf("Argument %d is %s \n", i, vargs[i]);
}

}

