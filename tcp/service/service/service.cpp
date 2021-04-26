#include <iostream>
#include <cstdio>
#include <Winsock2.h>
#include<string.h>
#include<ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

void sendfile(SOCKET sockConn) {
	char *recvbuf=new char[100];
	recv(sockConn, recvbuf, 100, 0);
	printf("%s\n",recvbuf);
	printf("请输入要传输的文件的名字并将文件放进程序工作目录\n");
	char filename[MAX_PATH];
	cin>>filename;

	send(sockConn, filename, strlen(filename), 0);
	HANDLE hf = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	
	DWORD nNumberOfBytesRead;
	int filesize = 0;
	filesize=GetFileSize(hf, NULL);

	char * file=new char[100];

	char *sizestr=new char[100];

	_itoa_s(filesize,sizestr, 100,10);
	//SetFilePointer(hf, 0, NULL, FILE_BEGIN);
	//ReadFile(hf, file, filesize, &nNumberOfBytesRead, NULL);
	//CloseHandle(hf);
	char* recvbuf2 = new char[100];	
	recv(sockConn, recvbuf2, 100, 0);
	printf("%s\n", recvbuf2);
	send(sockConn, sizestr, strlen(sizestr), 0);
	char* recvbuf3 = new char[100];
	recv(sockConn, recvbuf3, 100, 0);
	printf("%s\n", recvbuf3);
	int recv3 = atoi(recvbuf3);


	if (recv3 == 0) {
		filesize = 0;
	}

	int sresult = 0;
	int scannum = 0;
	while (sresult < filesize) {
		SetFilePointer(hf, 100*scannum, NULL, FILE_BEGIN);
		ReadFile(hf, file, 100, &nNumberOfBytesRead, NULL);
		scannum++;
		send(sockConn, file, 100, 0);
	}
	return;
}

void cmdsend(SOCKET sockConn) {
	char* recvbuf = new char[100];
	recv(sockConn, recvbuf, 100, 0);
	printf(recvbuf);
	delete[] recvbuf;
	char* cmdbuf = new char[100];
	cin >> cmdbuf; 
	char* cmdsize = new char[100];
	int size = strlen(cmdbuf);
	_itoa_s(size, cmdsize, 100, 10);
	send(sockConn, cmdsize, 100, 0);
	send(sockConn, cmdbuf,size , 0);
	delete[] cmdbuf;
	char* filesize = new char[100];
	recv(sockConn, filesize, 100, 0);
	int recvsize = atoi(filesize);
	int rresult = 0;
	char* returnbuf = new char[100];
	while (rresult<recvsize) {
		rresult += recv(sockConn, returnbuf, 100, 0);
		printf("%s", returnbuf);
	}


	return;
}

int main()
{
	// 加载socket动态链接库(dll)
	WORD wVersionRequested;

	WSADATA wsaData;	// 这结构是用于接收Windows Socket的结构信息的
	int err;

	wVersionRequested = MAKEWORD(1, 1);	// 请求1.1版本的WinSock库

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		return -1;			// 返回值为零的时候是表示成功申请WSAStartup
	}

	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1) {
		// 检查这个低字节是不是1，高字节是不是1以确定是否我们所请求的1.1版本
		// 否则的话，调用WSACleanup()清除信息，结束函数
		WSACleanup();
		return -1;
	}

	// 创建socket操作，建立流式套接字，返回套接字号sockSrv
	// SOCKET socket(int af, int type, int protocol);
	// 第一个参数，指定地址簇(TCP/IP只能是AF_INET，也可写成PF_INET)
	// 第二个，选择套接字的类型(流式套接字)
	// 第三个，特定地址家族相关协议（0为自动）
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	// 套接字sockSrv与本地地址相连
	// int bind(SOCKET s, const struct sockaddr* name, int namelen);
	// 第一个参数，指定需要绑定的套接字；
	// 第二个参数，指定该套接字的本地地址信息，该地址结构会随所用的网络协议的不同而不同
	// 第三个参数，指定该网络协议地址的长度

	// PS: struct sockaddr{ u_short sa_family; 
	//						char sa_data[14];};
	//                      sa_family指定该地址家族， sa_data起到占位占用一块内存分配区的作用
	//     在TCP/IP中，可使用sockaddr_in结构替换sockaddr，以方便填写地址信息
	// 
	//     struct sockaddr_in{ short sin_family; unsigned short sin_port; struct in_addr sin_addr; char sin_zero[8];};
	//     sin_family表示地址族，对于IP地址，sin_family成员将一直是AF_INET。
	//     sin_port指定将要分配给套接字的端口。
	//     sin_addr给出套接字的主机IP地址。
	//     sin_zero[8]给出填充数，让sockaddr_in与sockaddr结构的长度一样。
	//     将IP地址指定为INADDR_ANY，允许套接字向任何分配给本地机器的IP地址发送或接收数据。
	//     如果想只让套接字使用多个IP中的一个地址，可指定实际地址，用inet_addr()函数。
	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000);

	bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)); // 第二参数要强制类型转换

	// 将套接字设置为监听模式（连接请求）， listen()通知TCP服务器准备好接收连接
	// int listen(SOCKET s,  int backlog);
	// 第一个参数指定需要设置的套接字，第二个参数为（等待连接队列的最大长度）
	listen(sockSrv, 10);

	// accept()，接收连接，等待客户端连接
	// SOCKET accept(  SOCKET s,  struct sockaddr* addr,  int* addrlen);
	// 第一个参数，接收一个处于监听状态下的套接字
	// 第二个参数，sockaddr用于保存客户端地址的信息
	// 第三个参数，用于指定这个地址的长度
	// 返回的是向与这个监听状态下的套接字通信的套接字

	// 客户端与用户端进行通信

	// send(), 在套接字上发送数据
	// int send( SOCKET s,  const char* buf,  int len,  int flags);
	// 第一个参数，需要发送信息的套接字，
	// 第二个参数，包含了需要被传送的数据，
	// 第三个参数是buffer的数据长度，
	// 第四个参数，一些传送参数的设置

	// recv(), 在套接字上接收数据
	// int recv(  SOCKET s,  char* buf,  int len,  int flags);
	// 第一个参数，建立连接后的套接字，
	// 第二个参数，接收数据
	// 第三个参数，接收数据的长度，
	// 第四个参数，一些传送参数的设置

	SOCKADDR_IN  addrClient;
	int len = sizeof(SOCKADDR);

	while (true) {	// 不断等待客户端请求的到来
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrClient, &len);
		addrClient.sin_addr;
		char* ipbuf = new char[46];
		inet_ntop(addrClient.sin_family, &addrClient.sin_addr, ipbuf, 46);
		char* sendBuf = new char[100];
		sprintf_s(sendBuf, 100, "Welcome %s to the server program~ \nNow, let's start talking...\n", ipbuf);
		delete[] ipbuf;
		send(sockConn, sendBuf, strlen(sendBuf) + 1, 0);
		delete[] sendBuf;
		int runsign = 1;
		char *recvBuf=new char[100];
		int recvnum = recv(sockConn, recvBuf, 100, 0);
		if (recvnum> 0) {
			printf("%s\n", recvBuf);
			runsign = 0;
		}
		delete[] recvBuf;
			// 接收第一次信息
		int num=0;
		char *sendnum =new char[100];
		while (runsign == 0) {		
			printf("请选择要进行的操作\n");
			printf("9-断开链接 1-传输文件 2-cmd命令\n");
			printf("\n");
			cin >> num;
			_itoa_s(num, sendnum,100,10);
			send(sockConn, sendnum, strlen(sendnum), 0);

			switch (num) {
			case 9:				
				runsign = 1;
				break;
			case 1:
				sendfile(sockConn);
				break;
			case 2:
				cmdsend(sockConn);
				break;
			default:
				break;
			}
		}
		delete[] sendnum;
		closesocket(sockConn);
	}

	printf("\n");
	system("pause");
	return 0;
}

