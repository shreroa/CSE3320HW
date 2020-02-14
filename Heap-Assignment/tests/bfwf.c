  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
    char * ptr1 = ( char * ) malloc ( 65535 );
    char * ptr4 = ( char * ) malloc ( 65 );
    char * ptr2 = ( char * ) malloc ( 1000 );
    char * ptr5 = ( char * ) malloc ( 75000 );
    char * ptr7 = ( char * ) malloc ( 30 );
    char * ptr6 = ( char * ) malloc ( 1100 );
    printf("Allocation Lists:\n%p\n%p\n%p\n%p\n%p\n%p\n",ptr1,ptr4,ptr2,ptr5,ptr7,ptr6);
    printf("Worst fit should pick this one: %p\n", ptr5 );
    printf("Best fit should pick this one: %p\n", ptr2 );

    free( ptr1 );
    free( ptr2 );
    free(ptr5);
    free(ptr6);

    ptr4 = ptr4;
    ptr7 = ptr7;

    char * ptr3 = ( char * ) malloc ( 1000 );
    printf("Chosen address: %p\n", ptr3 );

  return 0;
}
