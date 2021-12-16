#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <signal.h>
#define SERVER_ADDR "127.0.0.1" 

const int BUFFSIZE=2048;
const int SERVER_PORT=50000;

//数据包格式
struct DATA
{
    int color; //落子颜色
    int row;   //列
    int col;   //行
    int stop;  //停止指示符
};

//将数据包序列化为char
inline void toChar(char str[],DATA &data){
    str[0]=data.color;
    str[1]=data.row;
    str[2]=data.col;
    str[3]=data.stop;
}

//将字符流反序列化为数据包
inline void toData(char str[],DATA &data){
    data.color=str[0];
    data.row=str[1];
    data.col=str[2];
}

int sockfd;
inline void stop(int p){
    close(sockfd);
    std::cout<<"[info] close client";
    exit(0);
}
int main()
{
    struct sockaddr_in servaddr;
    char buff[BUFFSIZE];
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        std::cout<<"[error] Create socket error("<<errno<<"):"<<strerror(errno)<<std::endl;
        return -1;
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_ADDR, &servaddr.sin_addr);
    servaddr.sin_port = htons(SERVER_PORT);
    if (-1 == connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)))
    {
        std::cout<<"[error] Connet error("<<errno<<"):"<<strerror(errno)<<std::endl;
        return -1;
    }
    for(;;){
        signal(SIGINT, stop);
        std::cout<<"Please input > ";
        int in;
        DATA data;
        std::cin>>data.color>>data.row>>data.col>>data.stop;
        bzero(buff, sizeof(buff));
        toChar(buff,data);
        send(sockfd, buff, strlen(buff), 0);
        recv(sockfd, buff, BUFFSIZE - 1, 0);
        toData(buff,data);
        if(data.stop==1){
            std::cout<<"[info] stop signal reveived from server"<<std::endl;
            stop(0);
        }
        std::cout<<data.color<<std::endl;
        std::cout<<data.row<<std::endl;
        std::cout<<data.col<<std::endl;
    }
    return 0;
}