#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<pthread.h>
#include<fcntl.h>
#include<dirent.h>
#define SER_PORT 6000
#define SER_IP "127.0.0.1"
#define PATH "/bin/"
#define GET 1
#define PUT 2

int socket_create();
void* thread_fun(void *arg);

void sendfile(char* buff,int c);
void recvfile(char* buff,int c);

typedef void(*PFUNC)(char*,int);

typedef struct 
{
    int choice;
    PFUNC pfunc;

}ctrl;

ctrl ct[]=
{
    {GET,sendfile},
    {PUT,recvfile},
};


int main()
{
    int sockfd = socket_create();
    struct sockaddr_in caddr;
    while(1)
    {
        int len = sizeof(caddr);
        int c = accept(sockfd,(struct  sockaddr*)&caddr,&len);

        if(c<0)
        {
            continue;
        }

        printf("accept c = %d\n",c);
        pthread_t id;
        pthread_create(&id,NULL,thread_fun,(void*)c);
    }
}

void* thread_fun(void *arg)
{
    int c = (int)arg;
    while(1)
    {
        char buff[128] = {0};
        int  n = recv(c,buff,127,0);
        if(n <= 0)
        {
            break;
        }
        
        char *myargv[5] = {0};
        char *ptr = NULL;
        int i = 0;
        myargv[i] = strtok_r(buff," ",&ptr);
        if(myargv[0] == NULL)continue;
        while(myargv[++i] = strtok_r(NULL," ",&ptr));
       
		
		if(strncmp(myargv[0],"get",3) == 0)
		{
			ct[0].pfunc(myargv[1],c);

		}
		else if(strncmp(myargv[0],"put",3) == 0)
		{
			ct[1].pfunc(myargv[1],c);
		}
		else
		{
			int fd[2];
			if(pipe(fd)<0)
			{
				printf("pipe create error\n");
				close(c);
				break;
			}

            pipe(fd);
			pid_t pid = fork();
			assert(pid != -1);
			
			if(pid == 0)
			{
				close(fd[0]);
				dup2(fd[1],1);
				dup2(fd[1],2);
				char path[256]={PATH};
				strcat(path,myargv[0]);
				execv(path,myargv);
				exit(0);
			}
			close(fd[1]);
			wait(NULL);
			char sendbuff[128]={"ok#"};
			read(fd[0],sendbuff+3,131);
			send(c,sendbuff,strlen(sendbuff),0);
			memset(buff,0,sizeof(buff));
			memset(sendbuff,0,sizeof(sendbuff));
            close(fd[0]);
		}
    }
	
	printf("one cli exit!\n");
    close(c);
}
void sendfile(char* name,int c)// 下载函数
{
    if(name == NULL)
    {
        return;
    }
   int fd = open(name,O_RDONLY); 
   if(fd == -1)
   {
       send(c,"filename no exist!",18,0);
       return;
   }

   int size = lseek(fd,0,SEEK_END);
   lseek(fd,0,SEEK_SET);

   char stbuff[32] = {0};
   sprintf(stbuff,"ok#%d",size);//写入到stbuff
   send(c,stbuff,strlen(stbuff),0);//发送

   char status[32] = {0};

    if(recv(c,status,31,0) <= 0)
    {
        close(fd);
        return;
    }

    if(strncmp(status,"ok",2) != 0)
    {
        close(fd);
        return ;
    }
    
    char sendbuff[256] = {0};
    int n =0;
    while((n=read(fd,sendbuff,256))>0)
    {
        send(c,sendbuff,n,0);
    }
    close(fd);

}

void recvfile(char* buff,int c) //用来接收文件
{
    if( buff == NULL ) //只有put命令
    {
        return ;
    }
	char*filename=buff;
	int size=0;
	char mdbuff[64]={0};
	recv(c,mdbuff,63,0);
    DIR *dp=opendir("/home/Vivian/ftp/cli");
	if(dp==NULL)
	{
		perror("error");
	}
	struct dirent*dir;
	while((dir=readdir(dp))!=NULL)
	{
		char*ptr=dir->d_name;


	int mfd[2];

	if(pipe(mfd)<0)
	{
		printf("error");
		break;
	}

	pipe(mfd);
	pid_t pid=fork();
	assert(pid!=-1);
	if(pid==0)
	{
		int i=0;
		char*myargv[5]={0};
		myargv[i]=strtok(ptr," ");
		close(mfd[0]);
		dup2(mfd[1],1);
   		dup2(mfd[1],2);
		strcat(path,myargv[0]);
		execv(path,myargv);
		exit(0);
	}
	close(mfd[1]);
	wait(NULL);
	
	
	}
	
   int fd = open(filename,O_WRONLY|O_CREAT,0600);
	char status[32]={0};
	if(recv(c,status,31,0)>0)
	{
		if(strncmp(status,"ok#",3) == 0)	
		{
			sscanf(status+3,"%d",&size);
			send(c,"ok#",3,0);
			
			int recvbuff[256] = {0};
			int cursize = 0;
			int num = 0;
			while((num = recv(c,recvbuff,255,0)) > 0)
			{
			   write(fd,recvbuff,num);
			   cursize += num;
			   if(cursize - size >= 0 )
			   {
				   close(fd);
				   printf("上传成功！\n");
				   break;
			   }
			}		
		}
		else
		{
			return;
		}	
	} 
    
}



int socket_create()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd == -1)
    {
        return -1;
    }

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SER_PORT);
    saddr.sin_addr.s_addr = inet_addr(SER_IP);

    int res = bind(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    if(res == -1)
    {
        return -1;
    }
    
    listen(sockfd,5);
    return sockfd;
}


