#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>

#define IPADDRESS "127.0.0.1"
#define PORT 6666
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 100

//函数声明
//创建套接字并进行绑定
int sock_bind(const char * ip,int port);

//IO多路复用epoll
void do_epoll(int listenfd);

//event handler 事件处理函数
void handle_events(int epollfd,struct epoll_event * events,int num,int listenfd,char * buf);

//处理接受到的连接
void handle_accept(int epollfd,int listenfd);

//读处理
void do_read(int epollfd,int fd,char *buf);

//写处理
void do_write(int epollfd,int fd,char *buf);

//添加事件
void add_event(int epollfd,int fd,int state);

//修改事件
void modify_event(int epollfd,int fd,int state);

//删除事件
void delete_event(int epollfd,int fd,int state);

int main(int argc,char * argv[] ) 
{
    int listenfd;
    listenfd=sock_bind(IPADDRESS,PORT);
    listen(listenfd,LISTENQ);
    do_epoll(listenfd);
    return 0;
}

int sock_bind(const char * ip,int port)
{
    int listenfd;
    struct sockaddr_in servaddr;
    listenfd =socket(AF_INET,SOCK_STREAM,0);
    if(listenfd==-1){
        perror("socket error:");
        exit(1);
    }    
    bzero( &servaddr,sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    inet_pton(AF_INET,ip,&servaddr.sin_addr);
    servaddr.sin_port=htons(port);
    if(bind(listenfd,(struct sockaddr *)&servaddr,sizeof(servaddr))==-1)
    {
        perror("bind error:");
        exit(1);
    }
    return listenfd;
}
void do_epoll(int listenfd)
{
    int epollfd;
    struct epoll_event  events[EPOLLEVENTS];
    int ret;
    char buf[MAXSIZE];
    memset(buf,0,MAXSIZE);
    //创建一个描述符
    epollfd=epoll_create(FDSIZE);
    //添加监听描述符事件
    add_event(epollfd,listenfd,EPOLLIN);
    while (1)
    {
        /* code */
        //获取已经准备好的描述符事件
        ret=epoll_wait(epollfd,events,EPOLLEVENTS,-1);
        handle_events(epollfd,events,ret,listenfd,buf);

    }
    close(epollfd);
}
void handle_events(int epollfd,struct epoll_event *events,int num,int listenfd,char *buf)
{
    int i;
    int fd;
    for(i=0;i<num;i++)
    {
        fd=events[i].data.fd;
        if((fd==listenfd)&&(events[i].events&EPOLLIN))
            handle_accept(epollfd,listenfd);
        else if(events[i].events&EPOLLIN)
            do_read(epollfd,fd,buf);
        else if(events[i].events&EPOLLOUT)
            do_write(epollfd,fd,buf);
     
        
    }
}

void handle_accept(int epollfd,int listenfd)
{
    int clifd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    clifd=accept(listenfd,(struct sockaddr *)&cliaddr,&cliaddrlen);
    if(clifd==-1)
        perror("accept error:");
    else
    {
        printf("accept a new client: %s: %d \n",inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);

        add_event(epollfd,clifd,EPOLLIN);
    }
    
}

void do_read(int epollfd,int fd,char *buf)
{
    int nread;
    nread=read(fd,buf,MAXSIZE);
    if(nread==-1)
    {
        perror("read error:");
        close(fd);
        delete_event(epollfd,fd,EPOLLIN);

    }
    else if (nread==0)
    {
        /* code */
        fprintf(stderr,"client close\n");
        close(fd);
        delete_event(epollfd,fd,EPOLLIN);
    }
    else
    {
        printf(" read message is %s \n",buf);
        modify_event(epollfd,fd,EPOLLOUT);
    }
    
    
}

void do_write(int epollfd,int fd,char * buf)
{
    int nwrite;
    nwrite=write(fd,buf,strlen(buf));
    if(nwrite==-1)
    {
        perror("write error:");
        close(fd);
        delete_event(epollfd,fd,EPOLLOUT);
    }
    else
    {
        modify_event(epollfd,fd,EPOLLIN);
    }
    memset(buf,0,MAXSIZE);
    
}

void add_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events=state;
    ev.data.fd=fd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&ev);

}
void delete_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events=state;
    ev.data.fd=fd;
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,&ev);

}
void modify_event(int epollfd,int fd,int state)
{
    struct epoll_event ev;
    ev.events=state;
    ev.data.fd=fd;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&ev);
}