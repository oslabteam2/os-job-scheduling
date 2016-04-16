#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <limits.h> 
#include <errno.h>  
#include "job.h"
#define DEBUG
#define FIFO_FILE "/tmp/myfifo" 

/* 
 * √¸¡Ó”Ô∑®∏Ò Ω
 *     stat
 */
void usage()
{
	printf("Usage: stat\n");		
}

int main(int argc,char *argv[])
{
	struct jobcmd statcmd;
	int fd;
	char string[10000]="\0";
	int length;
	int out;
	int fd1;

	if(argc!=1)
	{
		usage();
		return 1;
	}

	statcmd.type=STAT;
	statcmd.defpri=0;
	statcmd.owner=getuid();
	statcmd.argnum=0;

	#ifdef DEBUG
		printf("statcmd cmdtype\t%d (-1 means ENQ, -2 means DEQ, -3 means STAT)\n"
			"statcmd owner\t%d\n"
			"statcmd defpri\t%d\n"
			"statcmd argnum\t%d\n",			
			statcmd.type,statcmd.owner,statcmd.defpri,statcmd.argnum);

    	#endif

	if((fd=open("/tmp/server",O_WRONLY))<0)
		error_sys("stat open fifo failed");

	if(write(fd,&statcmd,DATALEN)<0)
		error_sys("stat write failed");

	if((fd1=open("/tmp/stat",O_RDONLY|O_NONBLOCK))<0){
		if(mkfifo("/tmp/stat",0777)<0){
			;		
		}
	}

	sleep(1);

	printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

	out=read(fd1,string,10000);
	printf("%s\n",string);

	close(fd1);
}
