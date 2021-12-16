#include<iostream>
#include<cstring>
#include<termio.h>
#include <sys/socket.h>
#include <sys/unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <signal.h>
using namespace std;

namespace net{
    #define SERVER_ADDR "127.0.0.1" 

    const int BUFFSIZE=4096;
    const int SERVER_PORT=50000;
    int sockfd;
    char buff[BUFFSIZE];

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
        str[1]=data.color;
        str[2]=data.row;
        str[3]=data.col;
        str[4]=data.stop;
    }

    //将字符流反序列化为数据包
    inline void toData(char str[],DATA &data){
        data.color=str[1];
        data.row=str[2];
        data.col=str[3];
        data.stop=str[4];
    }

    inline void stop(int p){
        close(sockfd);
        std::cout<<"[info] close client";
        exit(0);
    }

    //连接server
    int init(){
        cout<<"[info] gomoku client started, connetting to server...";
        struct sockaddr_in servaddr;
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
        return 0;
    }
    
    //向server发包
    void push(int color,int row,int col,int sstop){
        int in;
        DATA data;
        data.color=color,data.row=row,data.col=col,data.stop=sstop;
        bzero(buff, sizeof(buff));
        toChar(buff,data);
        send(sockfd, buff, BUFFSIZE -1 , 0);
    }

    //向server收包
    DATA pull(){
        DATA data;
        recv(sockfd, buff, BUFFSIZE - 1, 0);
        toData(buff,data);
        if(data.stop==1){
            std::cout<<"[info] stop signal reveived from server"<<std::endl;
            stop(0);
        }
        return data;
    }
}

namespace key{
    int scan()
    {
        int in;
        struct termios t1;
        struct termios t2;
        tcgetattr(0, &t2);
        t1 = t2;
        t1.c_lflag &= (~ICANON);
        t1.c_cc[VTIME] = 0;
        tcgetattr(0, &t2);
        t1.c_cc[VMIN] = 1;
        tcsetattr(0, TCSANOW, &t1);

        in = getchar();

        tcsetattr(0, TCSANOW, &t2);
        return in;
    }

}

namespace gmk
{
    const int N=20,INF=0x3f3f3f3f;
    int matrix[N][N];//1为白子，0为黑子
    int mvr[N]={0,-1,1,-1,1,-1,1,0,0};
    int mvc[N]={0,0,0,-1,1,1,-1,-1,1};
    int color=0;//我方颜色
    int round=1;//当前轮数
    int n=15;//棋盘大小
    int cursorr,cursorc;//光标位置

    void init(){
        memset(matrix,0x3f,sizeof matrix);
        net::init();
        color=net::pull().color;
        cout<<"[info] connet to server successfully"<<endl;
    }

    inline bool check(int row,int col,int color){
        return row>=1&&row<=n&&col>=1&&col<=n&&matrix[row][col]==color;
    }

    bool win(int row,int col,int color){
        //先寻找端点，然后计算最长连段
        int res=0;
        for(int i=1;i<=8;i+=2){
            int cnt=0;
            int r=row,c=col;
            while(check(r+mvr[i],c+mvc[i],color)) r+=mvr[i],c+=mvc[i];
            while(check(r,c,color)) cnt++,r+=mvr[i+1],c+=mvc[i+1];
            res=max(res,cnt);
        }
        return res>=5;
    }

    void print(){
        system("clear");
        for(int i=0;i<=n*2;i++){
            for(int j=0;j<=n*2;j++){
                if(!(i&1)&&!(j&1)) cout<<'+';
                else if(!(i&1)&&j&1) cout<<"---";
                else if(i&1&&!(j&1)){
                    if(j==0&&j+1==cursorc&&i==cursorr) cout<<"|>";
                    else if(j==0) cout<<"| ";
                    else if(j+1==cursorc&&i==cursorr) cout<<" |>";
                    else if(j==cursorc+1&&i==cursorr) cout<<"<| ";
                    else cout<<" | ";
                }
                else{
                    int row=(i+1)/2,col=(j+1)/2;
                    if(matrix[row][col]==0) cout<<"○";
                    else if(matrix[row][col]==1) cout<<"●";
                    else cout<<' ';
                }
            }
            cout<<endl;
        }
        string str;
        if(round&1) str="黑棋"; else str="白棋";
        if(color) cout<<"[ 白 ● ]"; else cout<<"[ 黑 ○ ]";
        cout<<"当前是"<<str<<"的回合 ";
        cout<<"Tip:按WASD 控制光标，空格或者回车落子"<<endl;
    }

    void start(){
        init();
        for(int i=0;;i^=1){
            signal(SIGINT, net::stop);
            int row,col;
            cursorr=cursorc=8;
            cursorc=cursorc*2-1;
            cursorr=cursorr*2-1;
            if(i==color){
                for(;;){
                    print();
                    int key=key::scan();
                    if(key==97&&cursorc>1) cursorc-=2;
                    else if(key==100&&cursorc<2*n-1) cursorc+=2;
                    else if(key==119&&cursorr>1) cursorr-=2;
                    else if(key==115&&cursorr<2*n-1) cursorr+=2;
                    else if(key==10||key==32){
                        row=(cursorr+1)/2,col=(cursorc+1)/2;
                        if(matrix[row][col]==INF) matrix[row][col]=color;
                        else continue;
                        print();
                        net::push(color,row,col,0);
                        break;
                    }
                }
                if(win(row,col,i)){
                    cout<<"胜利!  轮数:"<<round<<endl;
                    net::stop(0);
                }
            }
            else{
                print();
                net::DATA data=net::pull();
                matrix[data.row][data.col]=i;
                print();
                if(win(data.row,data.col,i)){
                    cout<<"失败qwq  轮数"<<round<<endl;
                    net::stop(0);
                }
            }
            round++;
        }
        
    }
}

int main(){
    gmk::start();
    return 0;
}