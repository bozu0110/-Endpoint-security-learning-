#include "stdafx.h"

void ServiceMain();
void ControlHandler(DWORD request);
void RunService();
void Shutdown();
BOOL IsInstall();
BOOL Install();
 
SERVICE_STATUS_HANDLE hServiceStatus;
SERVICE_STATUS status;

int WINAPI WinMain( __in HINSTANCE hInstance, __in_opt HINSTANCE hPrevInstance, __in_opt LPSTR lpCmdLine, __in int nShowCmd )
{
	SERVICE_TABLE_ENTRY st[] = 
	{
		{TEXT("calc"), (LPSERVICE_MAIN_FUNCTION)ServiceMain},
		{NULL, NULL}
	};

	if (!IsInstall())
	{
		Install();
	}

	StartServiceCtrlDispatcher(st);

	RunService();

	return 0;
}

void RunService()
{
	SC_HANDLE schSCManager;
	SC_HANDLE schService;

	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		MessageBox(NULL, TEXT("OpenSCManager error"), TEXT("error"), MB_OK | MB_ICONERROR);
		return;
	}

	schService = OpenService(schSCManager, TEXT("calc"), SERVICE_ALL_ACCESS);
	if (!schService)
	{
		MessageBox(NULL, TEXT("OpenService error"), TEXT("error"), MB_OK | MB_ICONERROR);
	}

	BOOL b = StartService(schService, 0, NULL);
}

void ControlHandler(DWORD request)
{
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		status.dwWin32ExitCode = 0;
		status.dwCurrentState = SERVICE_STOPPED;
		break;
	default:
		break;
	}
	SetServiceStatus(hServiceStatus, &status);
}

void ServiceMain()
{
	status.dwServiceType = SERVICE_WIN32;
	status.dwCurrentState = SERVICE_START_PENDING;
	status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	status.dwWin32ExitCode = 0;
	status.dwServiceSpecificExitCode = 0;
	status.dwCheckPoint = 0;
	status.dwWaitHint = 0;

	hServiceStatus = RegisterServiceCtrlHandler(TEXT("calc"), (LPHANDLER_FUNCTION)ControlHandler);
	if ((SERVICE_STATUS_HANDLE)0 == hServiceStatus)
	{
		return;
	}

	status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hServiceStatus, &status);
	Shutdown();
}

void Shutdown()
{
	OSVERSIONINFO os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);
	if (VER_PLATFORM_WIN32_NT == os.dwPlatformId)
	{
		HANDLE handle;
		TOKEN_PRIVILEGES tkp;

		OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &handle);
		LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		AdjustTokenPrivileges(handle, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);

		if (GetLastError() != ERROR_SUCCESS)
		{
			MessageBox(NULL, TEXT("AdjustTokenPrivileges error"), TEXT("error"), MB_OK | MB_ICONERROR);
		}
	}

	system("calc");
}

BOOL IsInstall()
{
	BOOL bResult = FALSE;

	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		SC_HANDLE schService = OpenService(schSCManager, TEXT("calc"), SERVICE_ALL_ACCESS);
		if (schService)
		{
			bResult = TRUE;
			CloseServiceHandle(schService);
		}
		CloseServiceHandle(schSCManager);
	}
	else
	{
		MessageBox(NULL, TEXT("OpenSCManager error"), TEXT("error"), MB_OK | MB_ICONERROR);
	}

	return bResult;
}

BOOL Install()
{
	SC_HANDLE schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!schSCManager)
	{
		MessageBox(NULL, TEXT("OpenSCManager error"), TEXT("error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	TCHAR szFilePath[MAX_PATH];
	GetModuleFileName(NULL, szFilePath, MAX_PATH);

	SC_HANDLE schService = CreateService(schSCManager, TEXT("calc"), TEXT("calc"), SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szFilePath, NULL, NULL, TEXT(""), NULL, NULL);
	if (!schService)
	{
		CloseServiceHandle(schSCManager);
		MessageBox(NULL, TEXT("CreateService error"), TEXT("error"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	CloseServiceHandle(schService);
	CloseServiceHandle(schSCManager);

	return TRUE;
}