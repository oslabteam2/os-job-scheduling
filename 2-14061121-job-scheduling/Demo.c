#include "time.h"
#include <stdio.h>

void main(int argc,char ** argv)
{
  time_t timer,timerc;
  int count=1;
  struct tm *timeinfo;
  time(&timer);//系统开始的时间
  while(1)
  {
     time(&timerc);
     if((timerc-timer)>=1)//每过1秒打印
     {
		if((timerc-timer)==1){
			if(argc>1)
            	printf("%s",argv[1]);
       		printf("程序经过%d秒\n",count++);
		}
       timer=timerc;
     }
  }
}
