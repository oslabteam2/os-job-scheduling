#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include "job.h"

#define DEBUG

int jobid=0;
int siginfo=1;
int fifo;
int globalfd;
int PriorityHeader[4]={0,2,1,0};
int TimeInterval[4]={0,5,2,1};
int chaosjobid[50][2]={0};
struct waitqueue *head[3];
int chaos=0;
struct waitqueue *next=NULL,*current =NULL;

/* ���ȳ��� */
void scheduler()
{
	char timebuf[BUFLEN];
	struct jobinfo *newjob=NULL;
	struct jobcmd cmd;
	int  count = 0;
	bzero(&cmd,DATALEN);
	if((count=read(fifo,&cmd,DATALEN))<0)
		error_sys("read fifo failed");
	/*#ifdef DEBUG
		printf("Reading whether other process send command!\n");
		if(count){
			printf("cmd cmdtype\t%d\ncmd defpri\t%d\ncmd data\t%s\n",cmd.type,cmd.defpri,cmd.data);
		}
		else
			printf("no data read\n");
	#endif*/

	/* ���µȴ������е���ҵ */
	/*#ifdef DEBUG
		printf("Update jobs in wait queue!\n");
	#endif*/
	updateall();
    	trim();
	switch(cmd.type){
	case ENQ:
		/*#ifdef DEBUG
			printf("Execute enq!\n");
		#endif*/
		do_enq(newjob,cmd);
		break;
	case DEQ:
		/*#ifdef DEBUG
			printf("Execute deq!\n");
		#endif*/
		do_deq(cmd); //gai dao zhe li le
		break;
	case STAT:
		/*#ifdef DEBUG
			printf("Execute stat!\n");
		#endif*/
		do_stat(cmd);
		break;
	default:
		break;
	}
	/* ѡ������ȼ���ҵ */

	/*#ifdef DEBUG
		printf("Select which job to run next!\n");
	#endif*/

	next=jobselect();
	/* ��ҵ�л� */
	/*#ifdef DEBUG
		printf("Switch to the next job!\n");
	#endif*/

	jobswitch();
}

int allocjid()
{
	return ++jobid;
}

void updateall()
{
	struct waitqueue *p, *prev,*stack;
	int headflag=0;
	char timebuf[BUFLEN];

/*	#ifdef DEBUG
		printf("before updatrall\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/
	/* ������ҵ����ʱ�� */
	if(current){
		current->job->run_time += 1; /* ��1����1000ms */
        	current->job->timer -= 1; //minus 1 for ending time slice
    	}
	/* ������ҵ�ȴ�ʱ�估���ȼ� */
	for(p = head[0]; p != NULL; p = p->next){
		p->job->wait_time += 1000;
	}
	for(prev = p = head[1]; p != NULL; prev = p, p = p->next){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 10000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
			chaosjobid[chaos][1]=1;
			chaosjobid[chaos++][0]=p->job->jid;

		}
	}
	for(prev = p = head[2],headflag=0; p != NULL; headflag=0){
		p->job->wait_time += 1000;
		if(p->job->wait_time >= 10000 && p->job->curpri < 3){
			p->job->curpri++;
			p->job->wait_time = 0;
			chaosjobid[chaos][1]=2;
			chaosjobid[chaos++][0]=p->job->jid;
		}
		prev = p, p = p->next;
	}

/*	#ifdef DEBUG
		printf("after updatrall\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/

}
void trim()
{
    if(chaos==0)
    {
        return ;
    }
    int i=0;
    for(i=0;i<chaos;i++)
    {
        struct waitqueue *p,*prev;
        int jid = chaosjobid[i][0];
        for(prev = p = head[chaosjobid[i][1]];p!=NULL;prev = p, p = p->next)
        {
            if(p->job->jid == jid)
            {
                if(prev == p)
                {
                    head[chaosjobid[i][1]] = head[chaosjobid[i][1]]->next;
                    p->next = NULL;
                    head[chaosjobid[i][1]-1] = AddToQueue(head[chaosjobid[i][1] - 1],p);
                }
                else
                {
                    prev->next=p->next;
                    p->next = NULL;
                    head[chaosjobid[i][1]-1] = AddToQueue(head[chaosjobid[i][1] - 1],p);
                }
                break;
            }
        }
    }
    chaos = 0;
}
struct waitqueue* jobselect()
{
	struct waitqueue *anext = NULL;
	char timebuf[BUFLEN];

	if(head[0]){
		/* �����ȴ������е���ҵ���ҵ����ȼ���ߵ���ҵ */
		anext = head[0];
		head[0]=head[0]->next;
		anext->next=NULL;
	}else if(head[1]){
		anext = head[1];
		head[1]=head[1]->next;
		anext->next=NULL;
	}else if(head[2]){
		anext = head[2];
		head[2]=head[2]->next;
		anext->next=NULL;
	}
/*	#ifdef DEBUG
		if(anext){
		printf("the selected job:\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");
		strcpy(timebuf,ctime(&(anext->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("selected \t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			anext->job->jid,
			anext->job->pid,
			anext->job->ownerid,
			anext->job->run_time,
			anext->job->wait_time,
			timebuf,"RUNNING",anext->job->curpri);
	}
	#endif*/
	return anext;
}

void jobswitch()
{
	struct waitqueue *p;
	int i;
	char timebuf[BUFLEN];

	if(current && current->job->state == DONE){ /* ��ǰ��ҵ��� */
		/* ��ҵ��ɣ�ɾ���� */
		for(i = 0;(current->job->cmdarg)[i] != NULL; i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i] = NULL;
		}
		/* �ͷſռ� */
		free(current->job->cmdarg);
		free(current->job);
		free(current);

		current = NULL;
	}


	if(next == NULL && current == NULL) /* û����ҵҪ���� */
        {return;}
	else if (next != NULL && current == NULL){ /* ��ʼ�µ���ҵ */

		printf("begin start new job\n");
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		kill(current->job->pid,SIGCONT);
		return;
	}
	else if (next != NULL && current != NULL){ /* �л���ҵ */
/*#ifdef DEBUG
	printf("before switch\n");
	printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("current \t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING",current->job->curpri);
	}

	for(p=head[0];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[1];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[2];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
#endif*/
        if(current->job->timer > 0)
        {
            head[PriorityHeader[next->job->curpri]]=AddToQueue(head[PriorityHeader[next->job->curpri]],next);
            return;
        }
        current->job->timer=TimeInterval[current->job->curpri];
		printf("switch to Pid: %d\n",next->job->pid);
		kill(current->job->pid,SIGSTOP);
		//current->job->curpri = current->job->defpri;
		current->job->wait_time = 0;
		current->job->state = READY;
        current->next = NULL;
		/* �Żصȴ����� */
		head[PriorityHeader[current->job->curpri]]=AddToQueue(head[PriorityHeader[current->job->curpri]],current);
		current = next;
		next = NULL;
		current->job->state = RUNNING;
		current->job->wait_time = 0;
		current->next=NULL;
		kill(current->job->pid,SIGCONT);
/*#ifdef DEBUG
	printf("after switch\n");
	printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("current \t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING",current->job->curpri);
	}

	for(p=head[0];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[1];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[2];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
#endif*/
		return;
	}else{ /* next == NULL��current != NULL�����л� */

		return;
	}
}

void sig_handler(int sig,siginfo_t *info,void *notused)
{
	int status;
	int ret;
	char timebuf[BUFLEN];
	struct waitqueue *p;
	switch (sig) {
case SIGALRM: /* �����ʱ�������õļ�ʱ��� */
	scheduler();
	/*#ifdef DEBUG
		printf("SIGVTALRM RECEIVED\n");
	#endif*/
	return;
case SIGCHLD: /* �ӽ��̽���ʱ���͸������̵��ź� */
	ret = waitpid(-1,&status,WNOHANG);
	if (ret == 0)
		return;
	if(WIFEXITED(status)){
		current->job->state = DONE;
		printf("normal termation, exit status = %d\n",WEXITSTATUS(status));
	}else if (WIFSIGNALED(status)){
		printf("abnormal termation, signal number = %d\n",WTERMSIG(status));
	}else if (WIFSTOPPED(status)){
		printf("child stopped, signal number = %d\n",WSTOPSIG(status));
	}
#ifdef DEBUG
	printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("current \t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING",current->job->curpri);
	}

	for(p=head[0];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[1];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[2];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
#endif
	return;
	default:
		return;
	}
}

void do_enq(struct jobinfo *newjob,struct jobcmd enqcmd)
{
	struct waitqueue *newnode,*p;
	int i=0,pid;
	char *offset,*argvec,*q;
	char **arglist;
	sigset_t zeromask;
	char timebuf[BUFLEN];

/*	#ifdef DEBUG
		printf("before enq\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/
	sigemptyset(&zeromask);
   	if(enqcmd.defpri==0)
        enqcmd.defpri=1;
	/* ��װjobinfo���ݽṹ */
	newjob = (struct jobinfo *)malloc(sizeof(struct jobinfo));
	newjob->jid = allocjid();
	newjob->defpri = enqcmd.defpri;
	newjob->curpri = enqcmd.defpri;
	newjob->ownerid = enqcmd.owner;
	newjob->state = READY;
	newjob->create_time = time(NULL);
	newjob->wait_time = 0;
	newjob->run_time = 0;
	newjob->timer = TimeInterval[enqcmd.defpri];
	arglist = (char**)malloc(sizeof(char*)*(enqcmd.argnum+1));
	newjob->cmdarg = arglist;
	offset = enqcmd.data;
	argvec = enqcmd.data;
	while (i < enqcmd.argnum){
		if(*offset == ':'){
			*offset++ = '\0';
			q = (char*)malloc(offset - argvec);
			strcpy(q,argvec);
			arglist[i++] = q;
			argvec = offset;
		}else
			offset++;
	}

	arglist[i] = NULL;

	#ifdef DEBUG

	printf("enqcmd argnum %d\n",enqcmd.argnum);
	for(i = 0;i < enqcmd.argnum; i++)
		printf("parse enqcmd:%s\n",arglist[i]);

	#endif

	/*��ȴ������������µ���ҵ*/
	newnode = (struct waitqueue*)malloc(sizeof(struct waitqueue));
	newnode->next =NULL;
	newnode->job=newjob;

    	int prioOrder=PriorityHeader[newjob->defpri];
	if(head[prioOrder])
	{
		for(p=head[prioOrder];p->next != NULL; p=p->next);
		p->next =newnode;
	}else
		{head[prioOrder]=newnode;}

	/*Ϊ��ҵ��������*/
	if((pid=fork())<0)
		error_sys("enq fork failed");

	if(pid==0){
		newjob->pid =getpid();
		/*�����ӽ���,�ȵ�ִ��*/
		raise(SIGSTOP);
	#ifdef DEBUG

		printf("begin running\n");
		for(i=0;arglist[i]!=NULL;i++)
			printf("arglist %s\n",arglist[i]);
	#endif

		/*�����ļ�����������׼���*/
		dup2(globalfd,1);
		/* ִ������ */
		if(execv(arglist[0],arglist)<0)
			printf("exec failed\n");
		exit(1);
	}else{
		newnode->job->pid=pid;

		if(current==NULL){
            if(head[prioOrder] == newnode) //warning ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
            {
                sleep(1);
                kill(pid,SIGCONT);
            }
		}
		else if(newjob->curpri > current->job->curpri)
        {
            kill(current->job->pid, SIGSTOP);
            current->job->timer=0;
        }
	}

/*	#ifdef DEBUG
		printf("after enq\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/
}

void do_deq(struct jobcmd deqcmd)
{
	int deqid,i;
	deqid=atoi(deqcmd.data);
	char timebuf[BUFLEN];
	struct waitqueue *newnode,*p;
/*	#ifdef DEBUG
		printf("before deq\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif

	#ifdef DEBUG
		printf("deq jid %d\n",deqid);
	#endif
*/
	/*current jodid==deqid,��ֹ��ǰ��ҵ*/
	if (current && current->job->jid ==deqid){
		printf("teminate current job\n");
		kill(current->job->pid,SIGKILL);
		for(i=0;(current->job->cmdarg)[i]!=NULL;i++){
			free((current->job->cmdarg)[i]);
			(current->job->cmdarg)[i]=NULL;
		}
		free(current->job->cmdarg);
		free(current->job);
		free(current);
		current=NULL;
	}
	else{ /* �����ڵȴ������в���deqid */
		for(i=0;i<3;i++){
            head[i] = RemoveFromQueue(head[i],deqid);
		}
	}
/*	#ifdef DEBUG
		printf("after deq\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/
}

void do_stat(struct jobcmd statcmd)
{
	struct waitqueue *p;
	char timebuf[BUFLEN];
	/*
	*��ӡ������ҵ��ͳ����Ϣ:
	*1.��ҵID
	*2.����ID
	*3.��ҵ������
	*4.��ҵ����ʱ��
	*5.��ҵ�ȴ�ʱ��
	*6.��ҵ����ʱ��
	*7.��ҵ״̬
	*/

	/* ��ӡ��Ϣͷ�� */

/*	#ifdef DEBUG
		printf("before stat\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/

	printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");
	if(current){
		strcpy(timebuf,ctime(&(current->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("current \t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			current->job->jid,
			current->job->pid,
			current->job->ownerid,
			current->job->run_time,
			current->job->wait_time,
			timebuf,"RUNNING",current->job->curpri);
	}

	for(p=head[0];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[1];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}
		for(p=head[2];p!=NULL;p=p->next){
		strcpy(timebuf,ctime(&(p->job->create_time)));
		timebuf[strlen(timebuf)-1]='\0';
		printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
			p->job->jid,
			p->job->pid,
			p->job->ownerid,
			p->job->run_time,
			p->job->wait_time,
			timebuf,
			"READY",
			p->job->curpri);
	}

/*	#ifdef DEBUG
		printf("after stat\n");
		printf("\t\tJOBID\tPID\tOWNER\tRUNTIME\tWAITTIME\tCREATTIME\t\tSTATE\tPRIO\n");

		for(p=head[0];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The3prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[1];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The2prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
		for(p=head[2];p!=NULL;p=p->next){
			strcpy(timebuf,ctime(&(p->job->create_time)));
			timebuf[strlen(timebuf)-1]='\0';
			printf("The1prioquue\t%d\t%d\t%d\t%d\t%d\t%s\t%s\t%d\n",
				p->job->jid,
				p->job->pid,
				p->job->ownerid,
				p->job->run_time,
				p->job->wait_time,
				timebuf,
				"READY",
				p->job->curpri);
		}
	#endif
*/
}

int main()
{
	struct timeval interval;
	struct itimerval new,old;
	struct stat statbuf;
	struct sigaction newact,oldact1,oldact2;
    head[0] = head[1] = head[2] = NULL;

	/*#ifdef DEBUG
		printf("DEBUG IS OPEN!");
	#endif*/

	if(stat("/tmp/server",&statbuf)==0){
		/* ���FIFO�ļ�����,ɾ�� */
		if(remove("/tmp/server")<0)
			error_sys("remove failed");
	}

	if(mkfifo("/tmp/server",0666)<0)
		error_sys("mkfifo failed");
	/* �ڷ�����ģʽ�´�FIFO */
	if((fifo=open("/tmp/server",O_RDONLY|O_NONBLOCK))<0)
		error_sys("open fifo failed");

	/* �����źŴ����� */
	newact.sa_sigaction=sig_handler;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags=SA_SIGINFO;
	sigaction(SIGCHLD,&newact,&oldact1);
	sigaction(SIGALRM,&newact,&oldact2);

	/* ����ʱ����Ϊ1000���� */
	interval.tv_sec=1;
	interval.tv_usec=0;

	new.it_interval=interval;
	new.it_value=interval;
	setitimer(ITIMER_REAL,&new,&old);

	while(siginfo==1);

	close(fifo);
	close(globalfd);
	return 0;
}
struct waitqueue *RemoveFromQueue(struct waitqueue* des, int Jobid)
{
    struct waitqueue *p=des,*prev;
    if(des==NULL)
        return des;
    else{
        int found = 0;
        for(prev=des,p=des;p!=NULL;prev = p,p = p->next)
        {
            if(p->job->jid == Jobid)
            {
                found = 1;
                break;
            }
        }
        if(found==1){
            if(p == des)
            {
                des = des->next;
                free(p->job->cmdarg);
                free(p->job);
                free(p);
            }
            else {
                prev->next=p->next;
                free(p->job->cmdarg);
                free(p->job);
                free(p);
            }
        }
        else{
            printf("No such jobid is found.\n");
        }
    }
    return des;
}
struct waitqueue *AddToQueue(struct waitqueue* des, struct waitqueue* tar)
{
    tar->next=NULL;
    if(des==NULL)
    {
        des = tar;
        des->next = NULL;
    }
    else
    {
        struct waitqueue *p,*prev;
        for(prev=des,p=des; p!=NULL;prev=p,p=p->next);
        prev->next = tar;
    }
    return des;
}
void print(struct waitqueue *tar)
{
    if(tar==NULL){
        printf("NULL\n");
    }
    else
    {
        struct waitqueue *p;
        printf("Waitqueue prio %d:\n",tar->job->curpri);
        for(p=tar;p!=NULL;p=p->next)
        {
            if(p->next==p)
                {
                printf("Dead loop.\n");
                return;
                }
            printf("Jobid:%d \n",p->job->jid);
        }
    }
}
