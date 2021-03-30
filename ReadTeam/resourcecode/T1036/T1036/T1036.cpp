// T1036.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
	STARTUPINFOA si = {0};
	si.cb = sizeof(STARTUPINFOA);
	PROCESS_INFORMATION pi;
	char ProcessName[MAX_PATH] = {0};
	StringCchCatA(ProcessName,MAX_PATH,"calc");
	CreateProcess(NULL,ProcessName, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
	return 0;
}

