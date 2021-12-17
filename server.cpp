#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <random>

namespace server{
    using namespace std;
    const int BUFFSIZE=4096;
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
    DATA data;
    char buff[BUFFSIZE];

    //将字符流反序列化为数据包
    inline void toData(char str[],DATA &data){
        data.color=str[1];
        data.row=str[2];
        data.col=str[3];
        data.stop=str[4];
    }

    //将数据包序列化为char
    inline void toChar(char str[],DATA &data){
        str[1]=data.color;
        str[2]=data.row;
        str[3]=data.col;
        str[4]=data.stop;
    }

    void quit(int p)
    {
        cout<<"[info] sendding stop signal to client"<<endl;
        data={0,0,0,1};
        toChar(buff,data);
        send(conn1,buff,BUFFSIZE - 1,0);
        send(conn2,buff,BUFFSIZE - 1,0);
        close(conn1);
        close(conn2);
        close(sock);
        cout<<"[info] shutting down..."<<endl;
        exit(0);
    }

    //初始化服务器连接
    int init(){
        cout<<"###########################"<<endl;
        cout<<"#####  Gomoku server  #####"<<endl;
        cout<<"###########################"<<endl;
        cout<<endl;
        struct sockaddr_in servaddr;

        sock=socket(AF_INET,SOCK_STREAM,0);
        if(sock==-1){
            cout<<"[error] Create socket error("<<errno<<"):"<<strerror(errno)<<endl;
            return -1;
        }
        bzero(&servaddr,sizeof servaddr);
        servaddr.sin_family=AF_INET;
        servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
        servaddr.sin_port=htons(PORT);
        
        if (bind(sock, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1){
            cout<<"[error] Bind socket error("<<errno<<"):"<<strerror(errno)<<endl;
            return -1;
        }

        if (listen(sock, MAXLINK)==-1)
        {
            cout<<"[error] Listen error("<<errno<<"):"<<strerror(errno)<<endl;
            return -1;
        }
        cout<<"[info] waitting..."<<endl;

        conn1=accept(sock,NULL,NULL);
        cout<<"[info] client[0] conneted"<<endl;
        conn2=accept(sock,NULL,NULL);
        cout<<"[info] client[1] conneted"<<endl;
        if(conn1==-1||conn2==-1){
            cout<<"[error] Accept error("<<errno<<"):"<<strerror(errno)<<endl;
            return -1;
        }
        int r=rand()%2;
        data={0,0,0,0};
        toChar(buff,data);
        if(r) send(conn1,buff,BUFFSIZE - 1,0);
        else send(conn2,buff,BUFFSIZE - 1,0);
        data={1,0,0,0};
        toChar(buff,data);
        if(r) send(conn2,buff,BUFFSIZE - 1,0);
        else send(conn2,buff,BUFFSIZE - 1,0);
        return 0;
    }

    int start(){
        if(init()) return -1;
        //主循环，i=1时为白子，i=0时为黑子
        for(int i=0;;i=i^1){
            signal(SIGPIPE, quit);
            signal(SIGINT,quit);
            bzero(buff, BUFFSIZE);
            if(i) recv(conn2,buff,BUFFSIZE-1,0);
            else recv(conn1,buff,BUFFSIZE-1,0);
            toData(buff,data);
            if(data.stop==1){
                cout<<"[info] stop signal reveived from client["<<i<<"] "<<endl;
                quit(0);
            }
            cout<<"[info] receive data successfully from client["<<i<<"] => ";
            cout<<"("<<data.row<<","<<data.col<<")"<<endl;
            
            if(i) send(conn1,buff,BUFFSIZE - 1,0);
            else send(conn2,buff,BUFFSIZE - 1,0);
        }
        return 0;
    }
}

int main(){
    return server::start();
}