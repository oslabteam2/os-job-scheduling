#include<stdio.h>
#include<stdlib.h>
int main(int argc,char **argv)
{
    int i = 0;
    for(i = 0; i < 5; i++)
    {
        sleep(3);
        if(argc>1)
            printf("%s",argv[1]);
        printf("The time is %i.\n",i*3);
    }
    exit(0);
}
