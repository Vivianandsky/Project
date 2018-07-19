#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<assert.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<pthread.h>

#define SER_PORT 6000
#define SER_IP "127.0.0.1"
int main()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd != -1);

    struct sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SER_PORT);
    saddr.sin_addr.s_addr = inet_addr(SER_IP);

    int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res != -1);

    while(1)
    {
        char buff[128] = {0};
        printf("Connect success>> ");
        fflush(stdout);
    
        fgets(buff,128,stdin);
        buff[strlen(buff)-1] = 0;
        if(strncmp(buff,"end",3) == 0)
        {
            break;
        }
		
        send(sockfd,buff,strlen(buff),0);
		
        char *s =strtok(buff," ");
        if(s == NULL)   //处理回车
        {
            continue;
        }
		if(strcmp(s,"a")==0)
		{
			pid_t pid=fork();
			if(pid==0)
			{
				printf("\n");
				char *argv[]={"ls","-l","/home/cf/project/clifile",0};
				execvp("ls",argv);
				//execlp("ls","-l",NULL);
			}
			wait(NULL);
			
		}
        if(strcmp(s,"get") == 0) //下载
        {
            s =strtok(NULL," ");
            if(s == NULL)   //处理zhiyouget的问题
            {
                printf("filename NULL\n");
                continue;
            }
			
			if(access(s,F_OK)==0)
			{
				strcat(s,"1");
			}
            int fd =open(s,O_WRONLY|O_CREAT,0600);
            char recvbuff[128] = {0};
            int num = 0;
            if(recv(sockfd,recvbuff,128,0)>0)
            {
                if(strncmp(recvbuff,"filename no exist!",18) == 0)//处理要下载的文件不存在问题
                {
                    printf("filename no exist!");
                }
                if(strncmp(recvbuff,"ok#",3)==0)//如果读到OK#，开始接受数据
                {
                    send(sockfd,"ok",2,0);//通知客户端可以下载

                    int size = 0;
                    sscanf(recvbuff+3,"%d",&size);//读取
                    int cursize =  0;
                    while((num = recv(sockfd,recvbuff,127,0)) > 0)
                    {
                         write(fd,recvbuff,num);
                        cursize += num;
                        if(cursize- size>=0)
                        {
                            close(fd);
                            printf("下载完成!!！\n");
                            break;
                        }
                        float f = cursize*100.0/size;
                        printf("down :%2.f%%\r",f);
                        fflush(stdout);
                    }
                }
                else
                {
                    printf("filename err!\n");
                    continue;
                }
            }
        }
		else if(strcmp(s,"put")==0)
        {
			
            s =strtok(NULL," ");
			if(s == NULL)
			{
				pid_t pid=fork();
				if(pid==0)
				{
					printf("\n");
					execlp("ls","clifile",NULL);
				}
			   //	printf("filename NULL\n");
            	wait(NULL);
				continue;	 
			}
			else
			{
				int mfd[2];
				if(pipe(mfd)<0)
				{
					printf("pipe create error");
					break;
				}
				pipe(mfd);
				pid_t pid=fork();
				assert(pid!=-1);
				
				if(pid==0)
				{
					char *myargv[5]={0};
					char*ptr=NULL;
					int i=0;
					myargv[i]=strtok_r(s," ",&ptr);
					close(mfd[0]);
					dup2(mfd[1],1);
					dup2(mfd[1],2);
					char path[256]={"md5sum"};
					strcat(path,myargv[0]);
					execv(path,myargv);
					exit(0);
				}
				close(mfd[1]);
				wait(NULL);
				char mdbuff[128]={0};
				read(mfd[0],mdbuff,128);
				char *mbuff=strtok(mdbuff," ");
				send(sockfd,mbuff,strlen(mbuff),0);
				memset(mdbuff,0,sizeof(mdbuff));
				memset(mbuff,0,sizeof(mbuff));
				close(mfd[0]);
		
			int fd = open(s,O_RDONLY);
			if(fd == -1)
			{
				send(sockfd,"err",3,0);
				continue;
			}
			int size = lseek(fd,0,SEEK_END);
			lseek(fd,0,SEEK_SET);
			char stbuff[32] = {0};
			sprintf(stbuff,"ok#%d",size);
			send(sockfd,stbuff,strlen(stbuff),0);
			
			char status[32] = {0};
			
			if(recv(sockfd,status,31,0) <= 0)
			{
				close(fd);
				continue;
			}
			
			if(strncmp(status,"ok#",3) != 0)
			{
				close(fd);
				continue;
			}
			
			char sendbuff[256] = {0};
			int n = 0;
            int cursize = 0;
			while((n=read(fd,sendbuff,256)) > 0)
			{
                cursize += n;
				send(sockfd,sendbuff,n,0);
                float f =cursize*100.0/size;
                printf("put : %2.f%%\r",f);
                fflush(stdout);
			}
			close(fd);	        
		}
		}
        else	//一般的命令ls,ps
        {
            recv(sockfd,buff,127,0);
            printf("%s\n",buff+3);
        }
        memset(buff,0,128);
    }
    close(sockfd);
}
