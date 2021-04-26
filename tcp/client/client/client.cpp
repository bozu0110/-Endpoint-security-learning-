// client.cpp
#include <iostream>
#include <cstdio>
#include <Winsock2.h>
#include<string.h>
#include <ws2tcpip.h>
#include "Shlwapi.h"

using namespace std;

//#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
#pragma comment(lib, "ws2_32.lib")
#pragma comment (lib, "Shlwapi.lib")

void recvfile(SOCKET sockclient) {
	send(sockclient, "客户端已准备好接受文件\n",strlen("客户端已准备好接受文件\n") + 1,0);
	char *filename=new char[MAX_PATH];
	recv(sockclient, filename, MAX_PATH, 0);

	if (PathFileExistsA("c:\\test")==FALSE) {
		system("md c:\\test");
	}
	char* strbuf = new char[MAX_PATH];
	strcpy_s(strbuf, MAX_PATH, "c:\\test\\");
	strcat_s(strbuf, MAX_PATH, filename);
	strcpy_s(filename, MAX_PATH, strbuf);
	delete[] strbuf;
	HANDLE hf=CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, NULL, NULL);
	delete[] filename;
	char* sizestr = new char[100];
	int iresult = 0;

	if (hf == INVALID_HANDLE_VALUE) {
		send(sockclient, "创建失败文件或已存在\n", strlen("创建失败文件或已存在\n") + 1, 0);
		recv(sockclient, sizestr, 100, 0);
		send(sockclient, "", strlen("") + 1, 0);
		recv(sockclient, sizestr, 100, 0);
	}
	else {
		send(sockclient, "客户端已创建文件\n", strlen("客户端已创建文件\n") + 1, 0);
		recv(sockclient, sizestr, 100, 0);
		int size = atoi(sizestr);
		char* filebuf = new char[100];
		int writetime = 0;
		send(sockclient, "客户端已写入文件\n", strlen("客户端已写入文件\n") + 1, 0);
		while (iresult < size) {
			iresult += recv(sockclient, filebuf, 100, 0);
			SetFilePointer(hf, 100*writetime, NULL, FILE_BEGIN);
			writetime++;
			DWORD nNumberOfByteswrite;
			WriteFile(hf, filebuf, 100, &nNumberOfByteswrite, NULL);
		}
		delete[] filebuf;
	}
	delete[] sizestr;
	CloseHandle(hf);
	return;
}

void recvcmd(SOCKET sockclient) {
	HANDLE hf = CreateFileA("C:\\test\\return.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	send(sockclient, "请输入命令行命令（默认目录为C:\\test\\）\n", strlen("请输入命令行命令（默认目录为C:\\test\\）\n") + 1, 0);
	char* cmdsize = new char[100];
	CloseHandle(hf);
	recv(sockclient, cmdsize, 100, 0);
	int size=atoi(cmdsize);
	delete[] cmdsize;
	char* cmdbuf = new char[size];

	char* cmd = new char[100];
	recv(sockclient, cmdbuf, size, 0);
	
	for (int strnum = 0; strnum < size; strnum++) {
		cmd[strnum] = cmdbuf[strnum];
	}
	for (int strnum = size; strnum <100; strnum++) {
		cmd[strnum] = '\0';
	}
	strcat_s(cmd, 100, ">C:\\test\\return.txt");
	char* returnbuf = new char[100];
	system(cmd);
	delete[] cmd;
	delete[] cmdbuf;
	hf = CreateFileA("C:\\test\\return.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	int filesize = GetFileSize(hf, NULL);
	int iresult = 0;
	int writetimes = 0;
	char* filebuf = new char[100];
	char* sendsize = new char[100];
	_itoa_s(filesize,sendsize,100,10);
	send(sockclient, sendsize, 100, 0);
	while (iresult < filesize) {		
		SetFilePointer(hf, 100 * writetimes, NULL, FILE_BEGIN);
		writetimes++;
		DWORD nNumberOfByteswrite;
		ReadFile(hf, filebuf, 100, &nNumberOfByteswrite, NULL);
		iresult += send(sockclient, filebuf, 100, 0);
	}
	


	return;
}

int main()
{
	// 加载socket动态链接库(dll)
	WORD wVersionRequested;
	WSADATA wsaData;	// 这结构是用于接收Wjndows Socket的结构信息的
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

	// 创建socket操作，建立流式套接字，返回套接字号sockClient
	// SOCKET socket(int af, int type, int protocol);
	// 第一个参数，指定地址簇(TCP/IP只能是AF_INET，也可写成PF_INET)
	// 第二个，选择套接字的类型(流式套接字)，第三个，特定地址家族相关协议（0为自动）
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

	// 将套接字sockClient与远程主机相连
	// int connect( SOCKET s,  const struct sockaddr* name,  int namelen);
	// 第一个参数：需要进行连接操作的套接字
	// 第二个参数：设定所需要连接的地址信息
	// 第三个参数：地址的长度
	SOCKADDR_IN addrSrv;
	inet_pton(AF_INET, "127.0.0.1", &addrSrv.sin_addr.S_un.S_addr);	// 本地回路地址是127.0.0.1;实际使用时更改服务器的IP地址 
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(6000); 
	int runsign = 1;
	while (runsign == 1) {
		int result = connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));
		if (result != SOCKET_ERROR) {
			runsign = 0;
		}
	}
	char *recvBuf=new char[100];
	int choicenum = 10;
	recv(sockClient, recvBuf, 100, 0);
	delete[] recvBuf;
	send(sockClient, "Attention: A Client has enter...\n", strlen("Attention: A Client has enter...\n") + 1, 0);
	char *choiceBuf=new char[100];
	while (runsign ==0) {
		choicenum = 10;
		recv(sockClient, choiceBuf, 100, 0);
		choicenum = atoi(choiceBuf);
		switch (choicenum) {
		case 9:
			runsign = 1;
			break;
		case 1:
			recvfile(sockClient);
			break;
		case 2:
			recvcmd(sockClient);
			break;
		default:
			break;
		}		
	}
	delete[] choiceBuf;
	closesocket(sockClient);
	WSACleanup();	// 终止对套接字库的使用
	return 0;
}
