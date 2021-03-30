#include <Windows.h>
#include <TlHelp32.h>
#include <stdio.h>
#include <iostream>
#include <psapi.h>
#include <tchar.h>
#include <string>
#include <atlconv.h>
#include "Softpub.h"
#include "Shlwapi.h"
#include <stdlib.h>
#pragma comment (lib, "wintrust")
#pragma comment (lib, "Shlwapi.lib")
using namespace std;
int filetype = 0;
int checkmark = 0;
wstring dllchecked[1024];
int dllcheckednum = 0;
int dllcheck(char* Name);


enum DLL_STATUS { DLL_NOT_FOUND, DLL_FOUND_UNSIGNED, DLL_FOUND_SIGNED };
enum DLL_LOCATION { DLL_WINDOWS_DIRECTORY, DLL_WINDOWS_16BIT_DIRECTORY, DLL_SYSTEM_DIRECTORY, DLL_EXE_DIRECTORY, DLL_PATH_VARIABLE, DLL_NUMBER_POSSIBLE_LOCATIONS };

UINT iSize = sizeof('\\');
void StringToWstring(std::wstring& szDst, std::string str);





int getBytes(char* _dst, size_t _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfBytesRead;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	//cout<<_offset_file<<" "<<_len<<endl;
	//fseek(fp, _offset_file, 0);
	if(ReadFile(hF, _dst, _len, &nNumberOfBytesRead, NULL)==TRUE)
		cnt = (int)& nNumberOfBytesRead;
	return cnt;
}

int getBytesW(WCHAR* _dst, size_t _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfBytesRead;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	//cout<<_offset_file<<" "<<_len<<endl;
	//fseek(fp, _offset_file, 0);
	if(ReadFile(hF, _dst, _len, &nNumberOfBytesRead, NULL)==TRUE)
		cnt = (int)& nNumberOfBytesRead;
	return cnt;
}
int setBytes(char* _src, int _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfBytesRead;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	WriteFile(hF, _src, _offset_file, &nNumberOfBytesRead, NULL);
	cnt = (int)& nNumberOfBytesRead;
	return cnt;
}


IMAGE_DOS_HEADER get_IMAGE_DOS_HEADER(HANDLE hF)
{
	char* buf = new char[sizeof(IMAGE_DOS_HEADER)];
	if (buf != NULL) {
		memset(buf, 0, sizeof(IMAGE_DOS_HEADER));

		getBytes(buf, sizeof(IMAGE_DOS_HEADER), 0, hF);
		//cout<<"dos:"<<&buf<<endl;

		return (IMAGE_DOS_HEADER)* ((IMAGE_DOS_HEADER*)buf);
	}
}

_IMAGE_NT_HEADERS64 get_IMAGE_NT_HEADER(HANDLE hF, LONG e_lfanew)
{
	char* buf = new char[sizeof(_IMAGE_NT_HEADERS64)];
	if (buf != NULL){
		memset(buf, 0, sizeof(_IMAGE_NT_HEADERS64));
		//cout<<sizeof(_IMAGE_NT_HEADERS64)<<endl;
		getBytes(buf, sizeof(_IMAGE_NT_HEADERS64), e_lfanew, hF);
		//cout<<"nt"<<&buf<<endl;
		return (_IMAGE_NT_HEADERS64)* ((_IMAGE_NT_HEADERS64*)buf);
	}
		
}

_IMAGE_NT_HEADERS get_IMAGE_NT_HEADER32(HANDLE hF, LONG e_lfanew)
{
	char* buf = new char[sizeof(_IMAGE_NT_HEADERS)];
	if (buf != NULL) {
		memset(buf, 0, sizeof(_IMAGE_NT_HEADERS));
		//cout<<sizeof(_IMAGE_NT_HEADERS64)<<endl;
		getBytes(buf, sizeof(_IMAGE_NT_HEADERS), e_lfanew, hF);
		//cout<<"nt"<<&buf<<endl;
		return (_IMAGE_NT_HEADERS)* ((_IMAGE_NT_HEADERS*)buf);
	}
	
}

void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER* _dst, size_t _cnt, HANDLE hF)
{
	int cnt = (int)_cnt;
	_IMAGE_DOS_HEADER dsh = get_IMAGE_DOS_HEADER(hF);
	long  offset = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64);
	getBytes((char*)_dst, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hF);
}
void get_IMAGE_SECTION_HEADERS32(_IMAGE_SECTION_HEADER* _dst, size_t _cnt, HANDLE hF)
{
	int cnt = (int)_cnt;
	_IMAGE_DOS_HEADER dsh = get_IMAGE_DOS_HEADER(hF);
	long  offset = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS);
	getBytes((char*)_dst, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hF);
}
unsigned int rva_To_fa(unsigned int rva, HANDLE hF)
//将相对虚拟地址转为文件偏移地址
{
	unsigned int fa = 0;
	_IMAGE_DOS_HEADER dsh = get_IMAGE_DOS_HEADER(hF);
	_IMAGE_NT_HEADERS64 nth = get_IMAGE_NT_HEADER(hF, dsh.e_lfanew);
	IMAGE_FILE_HEADER fh = nth.FileHeader;

	int sectionCnt;
	sectionCnt = fh.NumberOfSections;
	size_t number_of_section = (size_t)sectionCnt;
	IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[ sectionCnt+1];
	get_IMAGE_SECTION_HEADERS(section_header, number_of_section, hF);
	if (rva < (dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + number_of_section * sizeof(IMAGE_SECTION_HEADER)))
		//rva还是在头部
	{
		return rva;
	}

	int sectionserial = 0;
	if (section_header != NULL) {
		for (sectionserial = 0; sectionserial < (number_of_section - 1); sectionserial++)
		{
			//cout<<section_header[i].VirtualAddress<<" "<<section_header[i+1].VirtualAddress<<endl;
			if (rva >= section_header[sectionserial].VirtualAddress && rva < section_header[sectionserial + 1].VirtualAddress)
				//夹在之间的,
			{
				break;
			}
		}
		int off = rva - section_header[sectionserial].VirtualAddress;
		fa = section_header[sectionserial].PointerToRawData + off;
	}
	return fa;
}

unsigned int rva_To_fa32(unsigned int rva, HANDLE hF)
//将相对虚拟地址转为文件偏移地址
{
	unsigned int fa = 0;
	_IMAGE_DOS_HEADER dsh = get_IMAGE_DOS_HEADER(hF);
	_IMAGE_NT_HEADERS nth = get_IMAGE_NT_HEADER32(hF, dsh.e_lfanew);
	int sectionCnt;
	sectionCnt = nth.FileHeader.NumberOfSections;
	size_t number_of_section = (size_t)sectionCnt;
	IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[ sectionCnt+1];
	get_IMAGE_SECTION_HEADERS32(section_header, number_of_section, hF);
	if (rva < (dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS) + number_of_section * sizeof(IMAGE_SECTION_HEADER)))
		//rva还是在头部
	{
		return rva;
	}

	int sectionserial = 0;
	if (section_header != NULL) {
		for (sectionserial = 0; sectionserial < (number_of_section - 1); sectionserial++)
		{
			//cout<<section_header[i].VirtualAddress<<" "<<section_header[i+1].VirtualAddress<<endl;
			if (rva >= section_header[sectionserial].VirtualAddress && rva < section_header[sectionserial + 1].VirtualAddress)
				//夹在之间的,
			{
				break;
			}
		}
		int off = rva - section_header[sectionserial].VirtualAddress;
		fa = section_header[sectionserial].PointerToRawData + off;
	}
	return fa;
}
void get_IMAGE_IMPORT_DESCRIPTORS(IMAGE_IMPORT_DESCRIPTOR* rst, HANDLE hF, _IMAGE_NT_HEADERS64 nth, int i) {
	getBytes((char*)rst, sizeof(IMAGE_IMPORT_DESCRIPTOR) * i, rva_To_fa(nth.OptionalHeader.DataDirectory[1].VirtualAddress, hF), hF);
}
void get_IMAGE_IMPORT_DESCRIPTORS32(IMAGE_IMPORT_DESCRIPTOR* rst, HANDLE hF, _IMAGE_NT_HEADERS nth, int i) {
	//cout << rva_To_fa32(nth.OptionalHeader.DataDirectory[1].VirtualAddress,hF) << endl;
	getBytes((char*)rst, sizeof(IMAGE_IMPORT_DESCRIPTOR) * i, rva_To_fa32(nth.OptionalHeader.DataDirectory[1].VirtualAddress, hF), hF);
}

int dlllist(char * NAME,string * str) {
	HANDLE hF = CreateFile(NAME, GENERIC_READ, FILE_SHARE_READ , NULL, OPEN_EXISTING, NULL, NULL);
	if (hF == INVALID_HANDLE_VALUE) {
		//std::cout <<NAME<< "未打开" << endl;
		checkmark = 2;
		return 0;
	}
	//导出调用的dll名字       
	_IMAGE_DOS_HEADER dsh = get_IMAGE_DOS_HEADER(hF);
	_IMAGE_NT_HEADERS64 nth = get_IMAGE_NT_HEADER(hF, dsh.e_lfanew);
	int import_count = 0;
	if (nth.FileHeader.SizeOfOptionalHeader == 240) {
		filetype = 64;
		import_count = nth.OptionalHeader.DataDirectory[1].Size / 20;
		IMAGE_IMPORT_DESCRIPTOR* IIMD =new IMAGE_IMPORT_DESCRIPTOR[ import_count];
		get_IMAGE_IMPORT_DESCRIPTORS(IIMD, hF, nth, import_count);
		char* bits = new char[2];
		if (IIMD != NULL) {
			for (int IIMDserial = 0; IIMDserial < import_count - 1; IIMDserial++) {
				for (int byteserial = 0; byteserial < 50; byteserial++) {
					getBytes(bits, 1, rva_To_fa(IIMD[IIMDserial].Name, hF) + byteserial, hF);
					if (*bits != NULL) {
						str[IIMDserial] += *bits;

					}
					else {
						break;
					}

				}
			}
		}
	}
	else {
		filetype = 32;
		_IMAGE_NT_HEADERS nth = get_IMAGE_NT_HEADER32(hF, dsh.e_lfanew);
		import_count = nth.OptionalHeader.DataDirectory[1].Size / 20;
		IMAGE_IMPORT_DESCRIPTOR* IIMD = new IMAGE_IMPORT_DESCRIPTOR[import_count];

		get_IMAGE_IMPORT_DESCRIPTORS32(IIMD, hF, nth, import_count);
		char* bits = new char[2];
		if (IIMD != NULL) {
			for (int IIMDserial = 0; IIMDserial < import_count - 1; IIMDserial++) {
				for (int byteserial = 0; byteserial < 50; byteserial++) {
					getBytes(bits, 1, rva_To_fa32(IIMD[IIMDserial].Name, hF) + byteserial, hF);
					if (*bits != NULL) {
						str[IIMDserial] += *bits;
					}
					else {
						break;
					}

				}
			}
		}
	}
	return import_count;
}

long issigned(LPCWSTR wPath) {

	/* Building various data structures used as part of the query */
	LONG lStatus;
	WINTRUST_FILE_INFO FileData;
	memset(&FileData, 0, sizeof(FileData));
	FileData.cbStruct = sizeof(WINTRUST_FILE_INFO);
	FileData.pcwszFilePath = wPath;
	FileData.hFile = NULL;
	FileData.pgKnownSubject = NULL;


	GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;
	WINTRUST_DATA WinTrustData;

	memset(&WinTrustData, 0, sizeof(WinTrustData));

	WinTrustData.cbStruct = sizeof(WinTrustData);
	WinTrustData.pPolicyCallbackData = NULL;
	WinTrustData.pSIPClientData = NULL;
	WinTrustData.dwUIChoice = WTD_UI_NONE;
	WinTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
	WinTrustData.dwUnionChoice = WTD_CHOICE_FILE;
	WinTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
	WinTrustData.hWVTStateData = NULL;
	WinTrustData.pwszURLReference = NULL;
	WinTrustData.dwUIContext = 0;

	WinTrustData.pFile = &FileData;

	/* API call which identifies whether a file has been correctly signed */
	lStatus = WinVerifyTrust(NULL, &WVTPolicyGUID, &WinTrustData);

	/* This function returns 0 if the file is correctly signed */
	return lStatus;
}


int check1(DLL_STATUS * dllSearch, WCHAR * tPathBeingChecked){
	if (PathFileExistsW(tPathBeingChecked) )
	{
		if (!issigned(tPathBeingChecked)) {
			dllSearch[DLL_EXE_DIRECTORY] = DLL_FOUND_SIGNED;	
			return 1;
		}
		else
			dllSearch[DLL_EXE_DIRECTORY] = DLL_FOUND_UNSIGNED;
		return 0;
	}
	return 2;
}
int check2(DLL_STATUS* dllSearch, WCHAR* tPathBeingChecked) {
	if (PathFileExistsW(tPathBeingChecked))
	{
		if (!issigned(tPathBeingChecked)) {
			dllSearch[DLL_WINDOWS_DIRECTORY] = DLL_FOUND_SIGNED;
			return 1;
		}
		else
			dllSearch[DLL_WINDOWS_DIRECTORY] = DLL_FOUND_UNSIGNED;
		return 0;
	}
	return 2;
}
int check3(DLL_STATUS* dllSearch, WCHAR* tPathBeingChecked) {
	if (PathFileExistsW(tPathBeingChecked))
	{
		if (!issigned(tPathBeingChecked)) {
			dllSearch[DLL_SYSTEM_DIRECTORY] = DLL_FOUND_SIGNED;
			return 1;
		}
		else
			dllSearch[DLL_SYSTEM_DIRECTORY] = DLL_FOUND_UNSIGNED;
		return 0;
	}
	return 2;
}
int check4(DLL_STATUS* dllSearch, WCHAR* tPathBeingChecked) {
	if (PathFileExistsW(tPathBeingChecked))
	{
		if (!issigned(tPathBeingChecked)) {
			dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] = DLL_FOUND_SIGNED;
			return 1;
		}
		else
			dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] = DLL_FOUND_UNSIGNED;
		return 0;
	}
	return 2;
}

int recheck(WCHAR * pFilename){
	for (int dllcheckedserial= 0; dllcheckedserial < dllcheckednum; dllcheckedserial++) {
		if(dllchecked[dllcheckedserial]==pFilename){
			return 0;
		}
	}
	//wcout << L"recheck " << pFilename << endl;
	dllchecked[dllcheckednum] = pFilename;
	dllcheckednum++;
	char name[260] = {0};
	for (int nameserial= 0; nameserial < wcslen(pFilename); nameserial++)
	{
		name[nameserial] = pFilename[nameserial];
	}
	string* DllList = new string[MAX_PATH];
	if (dlllist(name, DllList) != 0) {
		if (dllcheck(name) == 1) {
			return 1;
		}
	}
	return 0;
}

WCHAR * getpath(WCHAR * pathmem, const wchar_t * add)
{
	WCHAR path[260];
	wcscpy_s(path, _countof(path), pathmem);
	if (add != NULL) {
		wcscat_s(path, _countof(path), add);
	}
	return path;
}


int trydllpath(WCHAR* mEntry_path, WCHAR* pFilename) {
	int patherrornum = 0;
	WCHAR* slash = new WCHAR[2];
	slash[0] = L'\\';
	int* bchanged = new int[1024];
	for (int changedserial = 0; changedserial < 1024; changedserial++) {
		bchanged[changedserial] = 0;
	}//初始化

	for (int dllcheckedserial= 0; dllcheckedserial < dllcheckednum; dllcheckedserial++) {
		if (dllchecked[dllcheckedserial] == pFilename) {
			return 0;
		}
	}
	dllchecked[dllcheckednum] = pFilename;
	dllcheckednum++;
	if (pFilename[0] == '\0' || pFilename == NULL) {
		return 0;
	}
	WCHAR* pathmem = new WCHAR[260];
	/* Usually C:\Windows\System32*/
	WCHAR* wSystemDirectory = new WCHAR[260];
	rsize_t destsz1 = GetSystemDirectoryW(pathmem, MAX_PATH);
	wSystemDirectory = getpath(pathmem, slash);

	//Usually C:\Windows 
	WCHAR* wWindowsDirectory = new WCHAR[260];
	rsize_t destsz2 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	wWindowsDirectory = getpath(pathmem, slash);
	//wWindowsDirectory[destsz2] = L'\\';
	//wWindowsDirectory[destsz2+1] = L'\0';
	//wcout << wWindowsDirectory << endl;
	// Usually C:\Windows\System 
	WCHAR* wSystemDirectory_16bit = new WCHAR[260];
	rsize_t destsz3 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	wSystemDirectory_16bit = getpath(pathmem, L"\\system\\");
	//syswow64
	WCHAR* tWow64Dir = new WCHAR[MAX_PATH];
	rsize_t destsz4 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	tWow64Dir = getpath(pathmem, L"syswow64\\");
	// Various directories delimitered by a semi-colon in the path environmental variable 
	WCHAR* wPathVariable = new WCHAR[32767];
	GetEnvironmentVariableW(L"PATH", wPathVariable, 32767);

	WCHAR* wExeDirectory = new WCHAR[MAX_PATH];
	wExeDirectory = getpath(mEntry_path, NULL);
	WCHAR* exename = wcsrchr(wExeDirectory, '\\');

	if (exename == NULL) {
		WCHAR* exename = new WCHAR[MAX_PATH];
		exename = getpath(mEntry_path, NULL);

		GetModuleFileNameW(NULL, wExeDirectory, MAX_PATH);
		(wcsrchr(wExeDirectory, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	}
	else {
		exename++;
		int exesize = wcslen(wExeDirectory);
		int namesize = wcslen(exename);
		for (int nameserial = 0; nameserial < namesize; nameserial++) {
			wExeDirectory[exesize - nameserial - 1] = '\0';
		}
	}
	//for (int i = 0; pFilename[i] != '\0'; i++) {
	//	wExeDirectory[exesize + i - 1] = pFilename[i];
	//}


	//exe路径（不包含exe名字）
	WCHAR* wDLLPath = new WCHAR[MAX_PATH];
	wDLLPath = getpath(mEntry_path, NULL);



	//结果存放
	DLL_STATUS dllSearch[DLL_NUMBER_POSSIBLE_LOCATIONS] = { DLL_NOT_FOUND };
	//dll是否存在标志
	BOOL bFoundInDLLSearchPath = FALSE;
	WCHAR* tPathBeingChecked = new WCHAR[MAX_PATH];

	

	WCHAR* wNextToken = NULL;
	WCHAR* wPathVariable_WorkingCopy = new WCHAR[32767];
	WCHAR* wFoundinPath = new WCHAR [260];
	wstring* wfoundinpath = new wstring[126];

	WCHAR* wSplitPath_check = new WCHAR[MAX_PATH];
	//程序同名路径检测
	tPathBeingChecked=getpath(wExeDirectory,NULL);
	tPathBeingChecked = getpath(tPathBeingChecked, pFilename);
	int pathchecked = 0;
	switch (check1(dllSearch, tPathBeingChecked)) {
	case 1:
		if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
			bchanged[pathchecked] = 1;
			pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径		

		}
		break;
	case 2:
		pathchecked++;
		break;
	default:
		break;
	}

	tPathBeingChecked = getpath(wWindowsDirectory, pFilename);
	//windows文件夹中进行寻找
	switch (check2(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 1) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径

			}
		}
		break;
	case 2:
		if (pathchecked == 1)
			pathchecked++;
		break;
	default:
		break;
	}
	
	//system文件夹中进行寻找	
	tPathBeingChecked = getpath(wSystemDirectory, pFilename);
	switch (check3(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 2) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径

			}
		}
		break;
	case 2:
		if (pathchecked == 2)
			pathchecked++;
		break;
	default:
		break;
	}
	//system(16bit)中查找
	tPathBeingChecked = getpath(wSystemDirectory_16bit, pFilename);

	switch (check4(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 3) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径
			}
		}
		break;
	case 2:
		if (pathchecked == 3)
			pathchecked++;
		break;
	default:
		break;
	}

	if (pathchecked == 4) {
		//path中查找	
		wPathVariable_WorkingCopy = wPathVariable;
		WCHAR* wSplitPath = wcstok_s(wPathVariable_WorkingCopy, L";", &wNextToken);
		
		while (wSplitPath != NULL)
		{
			
			wSplitPath_check=getpath (wSplitPath,NULL);
			if (wSplitPath[(wcslen(wSplitPath) - 1)] != '\\')
				wSplitPath_check=getpath(wSplitPath_check, slash);
			
			if (!_wcsicmp(wSplitPath_check, wWindowsDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}
			

			//Check it's not the system directory (i.e. C:\Windows\System32\)
			if (!_wcsicmp(wSplitPath_check, wSystemDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			//Check it's not the 16 bit System directory (i.e. C:\Windows\System\)
			if (!_wcsicmp(wSplitPath_check, wSystemDirectory_16bit))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			// Check it's not the path the actual executable file sits in 
			if (!_wcsicmp(wSplitPath_check, wExeDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}
			
			// Create variable which is full path to libary which is being checked 
			tPathBeingChecked = getpath(wSplitPath, NULL);

			// Only append a final backslash if there isn't one there already 
			if (tPathBeingChecked[(wcslen(tPathBeingChecked) - 1)] != '\\') {
				tPathBeingChecked[(wcslen(tPathBeingChecked) - 1)]='\\';
			}
			tPathBeingChecked = getpath(tPathBeingChecked, pFilename);
			
			//Check whether that file exists 
			if (PathFileExistsW(tPathBeingChecked))
			{
			
				// If it exists, check whether it is digitally signed 
				if (!issigned(tPathBeingChecked))
				{
					// Update the fact we've found one, and append to the results string 
					// There may be more than one found from the path variable
					//	In order for unsigned-only mode to work, we shouldn't
					//	overwrite if we've found an UNSIGNED one previously 
					if (dllSearch[DLL_PATH_VARIABLE] != DLL_FOUND_UNSIGNED)
						dllSearch[DLL_PATH_VARIABLE] = DLL_FOUND_SIGNED;
					if (recheck(tPathBeingChecked) == 1) {
						wFoundinPath = getpath(tPathBeingChecked,NULL);
						wFoundinPath = getpath(wFoundinPath, L"[SIGNED]");
						bchanged[pathchecked] = 1;
						wFoundinPath = getpath(wFoundinPath, L"use unsigned DLL\n");	
						wfoundinpath[patherrornum] = wFoundinPath;
						patherrornum++;
					}
				}
				else
				{
					//Update the fact we've found one, and append to the results wcsing 
					dllSearch[DLL_PATH_VARIABLE] = DLL_FOUND_UNSIGNED;
					wFoundinPath = getpath(tPathBeingChecked, NULL);
					wFoundinPath= getpath(wFoundinPath, L"[UNSIGNED]\n");
					wfoundinpath[patherrornum] = wFoundinPath;
					patherrornum++;
				}		
			}
			//Move to next delimited item 
			wSplitPath = wcstok_s(NULL, L";", &wNextToken);	
		}
	}
	
	// Ignore SysWow64, otherwise you get a TON of matches 
	//WCHAR tWow64Dir[MAX_PATH];
	//wcscpy_t(tWow64Dir, wWindowsDirectory);
	//wcscat_t(tWow64Dir, L"syswow64\\");

	int iUnsignedFound = 0;
	int iSignedFound = 0;

	// For each of the possible locations, talley how many signed and unsigned we've found
	for (int dllresult = 0; dllresult < DLL_NUMBER_POSSIBLE_LOCATIONS; dllresult++)
		if (dllSearch[dllresult] == DLL_FOUND_SIGNED)
			iSignedFound++;
		else if (dllSearch[dllresult] == DLL_FOUND_UNSIGNED)
			iUnsignedFound++;
	
	//If it's found in two or more places (or at least one is unsigned is /unsigned mode is active) 
	if (((iSignedFound + iUnsignedFound) > 1) || (iUnsignedFound > 0))
	{

		//wprintf(L"\n%s可能被劫持\n\n", pFilename);

		if (iUnsignedFound > 0)
			wprintf(L"DLL可能被替换：%s\n\n", pFilename);
		//if((iSignedFound + iUnsignedFound) > 1)
		//	printf("存在同名DLL的地址:\n");
		if (dllSearch[DLL_EXE_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[0] == 1)
			wprintf(L"程序所在路径:%s%s [SIGNED]\n\n", wExeDirectory, pFilename);
		else if (dllSearch[DLL_EXE_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"程序所在路径:%s%s [UNSIGNED]\n\n", wExeDirectory, pFilename);


		if (dllSearch[DLL_SYSTEM_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[1] == 1)
			wprintf(L"系统路径:%s%s [SIGNED]\n\n", wSystemDirectory, pFilename);
		else if (dllSearch[DLL_SYSTEM_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"系统路径:%s%s [UNSIGNED]\n\n", wSystemDirectory, pFilename);

		if (dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[2] == 1)
			wprintf(L"系统路径(16 bit):%s%s [SIGNED]\n\n", wSystemDirectory_16bit, pFilename);
		else if (dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"系统路径(16 bit):%s%s [UNSIGNED]\n\n", wSystemDirectory_16bit, pFilename);


		if (dllSearch[DLL_WINDOWS_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[3] == 1)
			wprintf(L"Windows 路径:%s%s [SIGNED]\n\n", wWindowsDirectory, pFilename);
		else if (dllSearch[DLL_WINDOWS_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"Windows 路径:%s%s [UNSIGNED]\n\n", wWindowsDirectory, pFilename);


		if (dllSearch[DLL_PATH_VARIABLE] != DLL_NOT_FOUND)
			for (int printnum = 0; printnum < patherrornum; printnum++) {
				if (wfoundinpath[printnum].length()) {
					wprintf(L"环境变量路径:");
					wcout << wfoundinpath[printnum] << endl;
				}
			}
					

		if (bchanged[0] == 1 || bchanged[1] == 1 || bchanged[2] == 1 || bchanged[3] == 1 || bchanged[4] == 1 || iUnsignedFound > 0) {
			checkmark = 1;
			printf("\n-------------------------------------------------------------------------------\n");
		}
	}
	if (iUnsignedFound > 0)
		return 1;
	return 0;
}

int trydllpath32(WCHAR* mEntry_path, WCHAR* pFilename) {
	int patherrornum = 0;
	WCHAR* slash = new WCHAR[2];
	slash[0] = L'\\';
	int* bchanged = new int[1024];
	for (int changedserial = 0; changedserial < 1024; changedserial++) {
		bchanged[changedserial] = 0;
	}

	for (int dllcheckedserial= 0; dllcheckedserial < dllcheckednum;  dllcheckedserial++) {
		if (dllchecked[dllcheckedserial] == pFilename) {
			return 0;
		}
	}
	dllchecked[dllcheckednum] = pFilename;
	dllcheckednum++;
	if (pFilename[0] == '\0' || pFilename == NULL) {
		return 0;
	}
	WCHAR *pathmem=new WCHAR[260];
	/* Usually C:\Windows\System32*/
	WCHAR *wSystemDirectory=new WCHAR[260];
	rsize_t destsz1 = GetSystemDirectoryW(pathmem, MAX_PATH);
	wSystemDirectory = getpath(pathmem, slash);
	
	//Usually C:\Windows 
	WCHAR *wWindowsDirectory=new WCHAR[260];
	rsize_t destsz2 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	wWindowsDirectory=getpath(pathmem,slash);
	//wWindowsDirectory[destsz2] = L'\\';
	//wWindowsDirectory[destsz2+1] = L'\0';
	//wcout << wWindowsDirectory << endl;
	// Usually C:\Windows\System 
	WCHAR * wSystemDirectory_16bit=new WCHAR[260];
	rsize_t destsz3 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	wSystemDirectory_16bit=getpath(pathmem,L"\\system\\");
	//syswow64
	WCHAR *tWow64Dir=new WCHAR[MAX_PATH];
	rsize_t destsz4 = GetWindowsDirectoryW(pathmem, MAX_PATH);
	tWow64Dir = getpath(pathmem, L"syswow64\\");
	// Various directories delimitered by a semi-colon in the path environmental variable 
	WCHAR * wPathVariable=new WCHAR[32767];
	GetEnvironmentVariableW(L"PATH", wPathVariable, 32767);
	
	WCHAR * wExeDirectory=new WCHAR[MAX_PATH];
	wExeDirectory=getpath(mEntry_path,NULL);
	WCHAR * exename= wcsrchr(wExeDirectory, '\\');

	if (exename == NULL) {
		WCHAR *exename=new WCHAR[MAX_PATH];
		exename = getpath(mEntry_path, NULL);

		GetModuleFileNameW(NULL, wExeDirectory, MAX_PATH);
		(wcsrchr(wExeDirectory, _T('\\')))[1] = 0; // 删除文件名，只获得路径字串
	}
	else {
		exename++;
		int exesize = wcslen(wExeDirectory);
		int namesize = wcslen(exename);
		for (int nameserial = 0; nameserial < namesize; nameserial++) {
			wExeDirectory[exesize - nameserial - 1] = '\0';
		}
	}
	//for (int i = 0; pFilename[i] != '\0'; i++) {
	//	wExeDirectory[exesize + i - 1] = pFilename[i];
	//}


	//exe路径（不包含exe名字）
	WCHAR * wDLLPath=new WCHAR[MAX_PATH];
	wDLLPath = getpath(mEntry_path, NULL);



	//结果存放
	DLL_STATUS dllSearch[DLL_NUMBER_POSSIBLE_LOCATIONS] = { DLL_NOT_FOUND };
	//dll是否存在标志
	BOOL bFoundInDLLSearchPath = FALSE;
	WCHAR *tPathBeingChecked=new WCHAR[MAX_PATH];

	//程序同名路径检测

	WCHAR* wNextToken = NULL;
	WCHAR *wPathVariable_WorkingCopy=new WCHAR[32767];
	WCHAR** wFoundinPath = new WCHAR * [126];
	for (int numofarray = 0; numofarray < 126; numofarray++) {
		wFoundinPath[numofarray] = new WCHAR[260];
		*wFoundinPath[numofarray] = { 0 };
	}
	WCHAR * wSplitPath_check =new WCHAR[MAX_PATH];
	//检测
	tPathBeingChecked = getpath(tWow64Dir, pFilename);
	int pathchecked = 0;
	switch (check1(dllSearch, tPathBeingChecked)) {
	case 1:
		if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
			bchanged[pathchecked] = 1;
			pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径		

		}
		break;
	case 2:
		pathchecked++;
		break;
	default:
		break;
	}
	if (pathchecked == 1 && bchanged[pathchecked] != 1) {
		tPathBeingChecked = getpath(wExeDirectory, pFilename);
		int pathchecked = 0;
		if (recheck(tPathBeingChecked) == 1 && check1(dllSearch, tPathBeingChecked)==1) {//有签名，检查其调用dll是否有签名
			bchanged[pathchecked] = 1;
		}// 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径			
	}

	tPathBeingChecked = getpath(wWindowsDirectory, pFilename);
	//windows文件夹中进行寻找
	switch (check2(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 1) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径

			}
		}
		break;
	case 2:
		if (pathchecked == 1)
			pathchecked++;
		break;
	default:
		break;
	}
	//system(16bit)中查找
	tPathBeingChecked = getpath(wSystemDirectory_16bit, pFilename);

	switch (check4(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 2) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径
			}
		}
		break;
	case 2:
		if (pathchecked == 2)
			pathchecked++;
		break;
	default:
		break;
	}
	//system文件夹中进行寻找	
	tPathBeingChecked = getpath(wSystemDirectory, pFilename);
	switch (check3(dllSearch, tPathBeingChecked)) {
	case 1:
		if (pathchecked == 3) {
			if (recheck(tPathBeingChecked) == 1) {//有签名，检查其调用dll是否有签名
				bchanged[pathchecked] = 1;
				pathchecked++; // 其调用dll有无签名的dll，查找其它路径检查是否存在同名路径

			}
		}
		break;
	case 2:
		if (pathchecked == 3)
			pathchecked++;
		break;
	default:
		break;
	}
	
	if (pathchecked == 4) {
		//path中查找	
		wPathVariable_WorkingCopy = wPathVariable;
		WCHAR* wSplitPath = wcstok_s(wPathVariable_WorkingCopy, L";", &wNextToken);
		while (wSplitPath != NULL)
		{
			wSplitPath_check=getpath(wSplitPath,NULL);
			if (wSplitPath[(wcslen(wSplitPath) - 1)] != '\\')


				wSplitPath_check=getpath(wSplitPath_check, slash);
			if (!_wcsicmp(wSplitPath_check, wWindowsDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			//Check it's not the system directory (i.e. C:\Windows\System32\)
			if (!_wcsicmp(wSplitPath_check, wSystemDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			//Check it's not the 16 bit System directory (i.e. C:\Windows\System\)
			if (!_wcsicmp(wSplitPath_check, wSystemDirectory_16bit))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			// Check it's not the path the actual executable file sits in 
			if (!_wcsicmp(wSplitPath_check, wExeDirectory))
			{
				wSplitPath = wcstok_s(NULL, L";", &wNextToken);
				continue;
			}

			// Create variable which is full path to libary which is being checked 
			tPathBeingChecked = getpath(wSplitPath, NULL);
		
			// Only append a final backslash if there isn't one there already 
			if (tPathBeingChecked[(wcslen(tPathBeingChecked) - 1)] != '\\'){
				tPathBeingChecked = getpath(tPathBeingChecked, slash);
			}
			tPathBeingChecked = getpath(tPathBeingChecked, pFilename);

			//Check whether that file exists 
			if (PathFileExistsW(tPathBeingChecked))
			{
				// If it exists, check whether it is digitally signed 
				if (!issigned(tPathBeingChecked))
				{
					// Update the fact we've found one, and append to the results string 
					// There may be more than one found from the path variable
					//	In order for unsigned-only mode to work, we shouldn't
					//	overwrite if we've found an UNSIGNED one previously 
					if (dllSearch[DLL_PATH_VARIABLE] != DLL_FOUND_UNSIGNED)
						dllSearch[DLL_PATH_VARIABLE] = DLL_FOUND_SIGNED;										
					if (recheck(tPathBeingChecked) == 1) {
						wFoundinPath[patherrornum] = getpath(tPathBeingChecked,NULL);
						wFoundinPath[patherrornum] = getpath(wFoundinPath[patherrornum], L"[SIGNED]");
						bchanged[pathchecked] = 1;
						wFoundinPath[patherrornum] = getpath(wFoundinPath[patherrornum], L"use unsigned DLL\n");
						patherrornum++;
					}										
				}
				else
				{
					//Update the fact we've found one, and append to the results wcsing 
					dllSearch[DLL_PATH_VARIABLE] = DLL_FOUND_UNSIGNED;
					wFoundinPath[patherrornum] = getpath(tPathBeingChecked,NULL);
					wFoundinPath[patherrornum] = getpath(wFoundinPath[patherrornum], L"[UNSIGNED]\n");
					patherrornum++;
				}
			}
			//Move to next delimited item 
			wSplitPath = wcstok_s(NULL, L";", &wNextToken);
		}
	}

	// Ignore SysWow64, otherwise you get a TON of matches 
	//WCHAR tWow64Dir[MAX_PATH];
	//wcscpy_t(tWow64Dir, wWindowsDirectory);
	//wcscat_t(tWow64Dir, L"syswow64\\");

	int iUnsignedFound = 0;
	int iSignedFound = 0;

	// For each of the possible locations, talley how many signed and unsigned we've found
	for (int dllresult = 0; dllresult < DLL_NUMBER_POSSIBLE_LOCATIONS; dllresult++)
		if (dllSearch[dllresult] == DLL_FOUND_SIGNED)
			iSignedFound++;
		else if (dllSearch[dllresult] == DLL_FOUND_UNSIGNED)
			iUnsignedFound++;

	//If it's found in two or more places (or at least one is unsigned is /unsigned mode is active) 
	if (((iSignedFound + iUnsignedFound) > 1) || (iUnsignedFound > 0))
	{

		//wprintf(L"\n%s可能被劫持\n\n", pFilename);
	
		if (iUnsignedFound > 0)
			wprintf(L"DLL可能被替换：%s\n\n", pFilename);
		//if((iSignedFound + iUnsignedFound) > 1)
		//	printf("存在同名DLL的地址:\n");
		if (dllSearch[DLL_EXE_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[0] == 1)
			wprintf(L"程序所在路径:%s%s [SIGNED]\n\n", wExeDirectory, pFilename);
		else if (dllSearch[DLL_EXE_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"程序所在路径:%s%s [UNSIGNED]\n\n", wExeDirectory, pFilename);


		if (dllSearch[DLL_SYSTEM_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[1] == 1)
			wprintf(L"系统路径:%s%s [SIGNED]\n\n", wSystemDirectory, pFilename);
		else if (dllSearch[DLL_SYSTEM_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"系统路径:%s%s [UNSIGNED]\n\n", wSystemDirectory, pFilename);

		if (dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[2] == 1)
			wprintf(L"系统路径(16 bit):%s%s [SIGNED]\n\n", wSystemDirectory_16bit, pFilename);
		else if (dllSearch[DLL_WINDOWS_16BIT_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"系统路径(16 bit):%s%s [UNSIGNED]\n\n", wSystemDirectory_16bit, pFilename);


		if (dllSearch[DLL_WINDOWS_DIRECTORY] == DLL_FOUND_SIGNED && bchanged[3] == 1)
			wprintf(L"Windows 路径:%s%s [SIGNED]\n\n", wWindowsDirectory, pFilename);
		else if (dllSearch[DLL_WINDOWS_DIRECTORY] == DLL_FOUND_UNSIGNED)
			wprintf(L"Windows 路径:%s%s [UNSIGNED]\n\n", wWindowsDirectory, pFilename);


		if (dllSearch[DLL_PATH_VARIABLE] != DLL_NOT_FOUND)
			for (int printnum = 0; printnum <= patherrornum; printnum++) {
				if (wcslen(wFoundinPath[printnum]) != 0) {
					wprintf(L"环境变量路径:");
					wcout << wFoundinPath[printnum];
				}
					
			}
			
		if (bchanged[0] == 1 || bchanged[1] == 1 || bchanged[2] == 1 || bchanged[3] == 1 || bchanged[4] == 1 || iUnsignedFound > 0) {
			checkmark = 1;
			printf("\n-------------------------------------------------------------------------------\n");
		}
	}
	if (iUnsignedFound > 0)
		return 1;
	return 0;
}


void StringToWstring(std::wstring& szDst, std::string str)
{
	std::string temp = str;
	int len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL, 0);
	wchar_t* wszUtf8 = new wchar_t[len + 1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, len);
	szDst = wszUtf8;
	std::wstring r = wszUtf8;
	delete[] wszUtf8;
}

int dllcheck(char * Name) {
	int checksigned = 0;
	WCHAR* wname = new WCHAR[260];
	*wname = { 0};
	//cout<<"检测：" << Name << endl;
	for (int nameserail = 0; nameserail < strlen(Name);  nameserail++)
	{
		wname[nameserail] = Name[nameserail];
	}
	for (int dllcheckedserial = 0; dllcheckedserial < dllcheckednum; dllcheckedserial++) {
		if (dllchecked[dllcheckedserial] == wname) {
			return 0;
		}
	}
	dllchecked[dllcheckednum] = wname;
	dllcheckednum++;
	string* DllList = new string[MAX_PATH];
	int import_count = 0;	
	import_count = dlllist(Name, DllList);	

	if (import_count == 0) {
		cout << "文件不存在"<< endl;
		checkmark = 2;
	}
	wstring* wDLLlist = new wstring[MAX_PATH];
	for (int dllserial = 0; dllserial < import_count; dllserial++) {
		//cout << DllList[i] << endl;
		StringToWstring(wDLLlist[dllserial], DllList[dllserial]);
		wchar_t* wcht = new wchar_t[wDLLlist[dllserial].size() + 1];
		wcht[wDLLlist[dllserial].size()] = 0;
		std::copy(wDLLlist[dllserial].begin(), wDLLlist[dllserial].end(), wcht);
		if(filetype==32)
			checksigned = trydllpath32(wname, wcht);
		else
			checksigned = trydllpath(wname, wcht);
	}
	return checksigned;
}

int main(int argc,char* argv[]) {

	char * Name=new char[260];	
	setlocale(LC_ALL, "");
	if (argc > 1) {
		 Name= argv[1];
	}
	else {
		printf("用法示范：dllcheck.exe exepath(c:\\windows\\system32\\clac.exe)");
		return 0;
	}
	dllcheck(Name);
	if (checkmark == 0)
		cout << "检测完毕，未测出dll劫持可能" << endl;
	else if (checkmark == 1)
		cout << "检测完毕" << endl;
	return 0;
}
