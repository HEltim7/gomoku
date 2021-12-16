#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <signal.h>

const int BUFFSIZE=2048;
const int PORT=50000;
const int MAXLINK=2048;
int sock,conn1,conn2;
struct DATA
{
    int color;
    int row;
    int col;
    int stop;
};

void quit(int p)
{
    std::cout<<"[info] sendding stop signal to client"<<std::endl;
    char buff[BUFFSIZE]={0,0,0,1};
    send(conn1,buff,strlen(buff),0);
    send(conn2,buff,strlen(buff),0);
    close(conn1);
    close(conn2);
    close(sock);
    std::cout<<"[info] shutting down..."<<std::endl;
    exit(0);
}

//将字符流反序列化为数据包
inline void toData(char str[],DATA &data){
    data.color=str[0];
    data.row=str[1];
    data.col=str[2];
    data.stop=str[3];
}

//初始化服务器连接
int init(){
    struct sockaddr_in servaddr;

    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock==-1){
        std::cout<<"[error] Create socket error("<<errno<<"):"<<strerror(errno)<<std::endl;
        return -1;
    }
    bzero(&servaddr,sizeof servaddr);
    servaddr.sin_family=AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port=htons(PORT);
    
    if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1){
        std::cout<<"[error] Bind socket error("<<errno<<"):"<<strerror(errno)<<std::endl;
        return -1;
    }

    if (listen(sock, MAXLINK)==-1)
    {
        std::cout<<"[error] Listen error("<<errno<<"):"<<strerror(errno)<<std::endl;
        return -1;
    }
    std::cout<<"[info] Listening..."<<std::endl;
    return 0;
}

int server(){
    char buff[BUFFSIZE];
    for(;;){
        conn1=accept(sock,NULL,NULL);
        conn2=accept(sock,NULL,NULL);
        if(conn1==-1||conn2==-1){
            std::cout<<"[error] Accept error("<<errno<<"):"<<strerror(errno)<<std::endl;
            return -1;
        }
        //主循环，i=1时为白子，i=0时为黑子
        for(int i=1;;i=i^1){
            signal(SIGPIPE, quit);
            signal(SIGINT,quit);
            bzero(buff, BUFFSIZE);
            if(i) recv(conn1,buff,BUFFSIZE-1,0);
            else recv(conn2,buff,BUFFSIZE-1,0);
            DATA data;
            toData(buff,data);
            if(data.stop==1){
                std::cout<<"[info] stop signal reveived from client["<<i<<"]"<<std::endl;
                quit(0);
            }
            std::cout<<"[info] receive data successfully from client["<<i<<"]"<<std::endl;
            std::cout<<data.color<<std::endl;
            std::cout<<data.row<<std::endl;
            std::cout<<data.col<<std::endl;
            if(i) send(conn2,buff,strlen(buff),0);
            else send(conn1,buff,strlen(buff),0);
        }
    }
}

int main()
{
    if(init()) return -1;
    if(server()) return -1;
    return 0;
}

/*
https://zhuanlan.zhihu.com/p/119085959
https://www.seny.xyz/archives/gobang
*/