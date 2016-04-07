#include <stdio.h>
#include <stdlib.h>
#define BUFLEN 100
#define GLOBALFILE "screendump"

enum jobstate{
    READY,RUNNING,DONE
};

enum cmdtype{
    ENQ=-1,DEQ=-2,STAT=-3
};
struct jobcmd{
    enum cmdtype type;
    int argnum;
    int owner;
    int defpri;
    char data[BUFLEN];
};

#define DATALEN sizeof(struct jobcmd)

struct jobinfo{
    int jid;              /* 作业ID */
    int pid;              /* 进程ID */
    char** cmdarg;        /* 命令参数 */
    int defpri;           /* 默认优先级 */
    int curpri;           /* 当前优先级 */
    int ownerid;          /* 作业所有者ID */
    int wait_time;        /* 作业在等待队列中等待时间 */
    time_t create_time;   /* 作业创建时间 */
    int run_time;         /* 作业运行时间 */
    enum jobstate state;  /* 作业状态 */
};

struct waitqueue{
    struct waitqueue *next;
    struct jobinfo *job;
};
struct waitqueue *RemoveFromQueue(struct waitqueue* des, int Jobid);
struct waitqueue *AddToQueue(struct waitqueue* des, struct waitqueue* tar);
void print(struct waitqueue *tar);
struct waitqueue *head[3];
struct waitqueue* jobselect()
{
	struct waitqueue *next;
	int highest = -1;

	next = NULL;
	if(head[0]){
		/* 遍历等待队列中的作业，找到优先级最高的作业 */
		next = head[0];
		head[0]=head[0]->next;
		next->next=NULL;
	}else if(head[1]){
		next = head[1];
		head[1]=head[1]->next;
		next->next=NULL;
	}else if(head[2]){
		next = head[2];
		head[2]=head[2]->next;
		next->next=NULL;
	}
	return next;
}
int main()
{
    struct waitqueue *new,*next;head[0]=NULL;
    new = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    new->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    new->job->jid=1;
    new->next=NULL;
    head[0] = AddToQueue(head[0],new);
        new = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    new->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    new->job->jid=3;
    new->next=NULL; head[0] = AddToQueue(head[0],new);
        new = (struct waitqueue*)malloc(sizeof(struct waitqueue));
    new->job = (struct jobinfo*)malloc(sizeof(struct jobinfo));
    new->job->jid=2;
    new->next=NULL;
    head[0] = AddToQueue(head[0],new);
    print(head[0]);
    print(head[0]);
    next=jobselect();
    print(head[0]);
    print(next);

}
void print(struct waitqueue *tar)
{
    if(tar==NULL){
        printf("NULL\n");
    }
    else
    {
        struct waitqueue *p;
        printf("Waitqueue:\n");
        for(p=tar;p!=NULL;p=p->next)
        {
            printf("Jobid:%d \n",p->job->jid);
        }
    }
}
struct waitqueue *RemoveFromQueue(struct waitqueue* des, int Jobid)
{
    struct waitqueue *p=des,*prev;
    if(des==NULL)
        return des;
    else{
        int found = 0;
        for(prev=des,p=des;p!=NULL; prev = p,p = p->next)
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
