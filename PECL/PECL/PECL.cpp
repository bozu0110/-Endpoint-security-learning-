// PECL.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>

#include <windows.h>
using namespace std;
int filetype = 0;
int getBytes(char* _dst, size_t _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfBytesRead;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	//cout<<_offset_file<<" "<<_len<<endl;
	//fseek(fp, _offset_file, 0);
	if (ReadFile(hF, _dst, _len, &nNumberOfBytesRead, NULL) == TRUE)
		cnt = (int)& nNumberOfBytesRead;
	return cnt;
}
int setBytes(char* _dst, size_t _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfByteswrite;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	if (WriteFile(hF, _dst, _len, &nNumberOfByteswrite, NULL) == TRUE)
		cnt = (int)& nNumberOfByteswrite;
	return cnt;
}

int getBytesW(WCHAR * _dst, size_t _len, long _offset_file, HANDLE hF) {
	int cnt = 0;
	DWORD nNumberOfBytesRead;
	SetFilePointer(hF, _offset_file, NULL, FILE_BEGIN);
	//cout<<_offset_file<<" "<<_len<<endl;
	//fseek(fp, _offset_file, 0);
	if (ReadFile(hF, _dst, _len, &nNumberOfBytesRead, NULL) == TRUE)
		cnt = (int)& nNumberOfBytesRead;
	return cnt;
}

void get_IMAGE_DOS_HEADER(char* buf, HANDLE hF)
{
	if (buf != NULL) {
		memset(buf, 0, sizeof(IMAGE_DOS_HEADER));
		getBytes(buf, sizeof(IMAGE_DOS_HEADER), 0, hF);
		//cout<<"dos:"<<&buf<<endl;	
	}
}

void get_IMAGE_NT_HEADER(char* buf, HANDLE hF, LONG e_lfanew)
{
	if (buf != NULL) {
		memset(buf, 0, sizeof(_IMAGE_NT_HEADERS64));
		//cout<<sizeof(_IMAGE_NT_HEADERS64)<<endl;
		getBytes(buf, sizeof(_IMAGE_NT_HEADERS64), e_lfanew, hF);
		//cout<<"nt"<<&buf<<endl;	
	}
}

void get_IMAGE_NT_HEADER32(char* buf, HANDLE hF, LONG e_lfanew)
{
	if (buf != NULL) {
		memset(buf, 0, sizeof(_IMAGE_NT_HEADERS));
		//cout<<sizeof(_IMAGE_NT_HEADERS64)<<endl;
		getBytes(buf, sizeof(_IMAGE_NT_HEADERS), e_lfanew, hF);
		//cout<<"nt"<<&buf<<endl;		
	}
}

void get_IMAGE_SECTION_HEADERS(_IMAGE_SECTION_HEADER * _dst, size_t _cnt, HANDLE hF)
{
	int cnt = (int)_cnt;
	char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
	get_IMAGE_DOS_HEADER(IDH, hF);
	_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
	delete[] IDH;
	long  offset = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64);
	getBytes((char*)_dst, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hF);
}

void get_IMAGE_SECTION_HEADERS32(_IMAGE_SECTION_HEADER * _dst, size_t _cnt, HANDLE hF)
{
	int cnt = (int)_cnt;
	char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
	get_IMAGE_DOS_HEADER(IDH, hF);
	_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
	delete[] IDH;
	long  offset = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS);
	getBytes((char*)_dst, sizeof(IMAGE_SECTION_HEADER) * cnt, offset, hF);
}

unsigned int rva_To_fa(unsigned int rva, HANDLE hF)
//将相对虚拟地址转为文件偏移地址
{
	unsigned int fa = 0;
	char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
	get_IMAGE_DOS_HEADER(IDH, hF);
	_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
	delete[] IDH;

	char* INH = new char[sizeof(_IMAGE_NT_HEADERS64)];
	get_IMAGE_NT_HEADER(INH, hF, dsh.e_lfanew);
	_IMAGE_NT_HEADERS64 nth = (_IMAGE_NT_HEADERS64) * ((_IMAGE_NT_HEADERS64*)INH);;
	delete[] INH;

	IMAGE_FILE_HEADER fh = nth.FileHeader;
	int sectionCnt;
	sectionCnt = fh.NumberOfSections;
	size_t number_of_section = (size_t)sectionCnt;
	IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[sectionCnt];
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
	delete[] section_header;
	return fa;

}

unsigned int rva_To_fa32(unsigned int rva, HANDLE hF)
//将相对虚拟地址转为文件偏移地址
{
	unsigned int fa = 0;

	char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
	get_IMAGE_DOS_HEADER(IDH, hF);
	_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
	delete[] IDH;

	char* INH = new char[sizeof(_IMAGE_NT_HEADERS)];
	get_IMAGE_NT_HEADER32(INH, hF, dsh.e_lfanew);
	_IMAGE_NT_HEADERS nth = (_IMAGE_NT_HEADERS) * ((_IMAGE_NT_HEADERS*)INH);;
	delete[] INH;

	int sectionCnt;
	sectionCnt = nth.FileHeader.NumberOfSections;
	size_t number_of_section = (size_t)sectionCnt;
	IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[sectionCnt];
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

unsigned int fa_To_rva(unsigned int fa,HANDLE hF)
	{
		unsigned int rva = 0;

		char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
		get_IMAGE_DOS_HEADER(IDH, hF);
		_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
		delete[] IDH;

		char* INH = new char[sizeof(_IMAGE_NT_HEADERS64)];
		get_IMAGE_NT_HEADER(INH, hF, dsh.e_lfanew);
		_IMAGE_NT_HEADERS64 nth = (_IMAGE_NT_HEADERS64) * ((_IMAGE_NT_HEADERS64*)INH);;
		delete[] INH;

		IMAGE_FILE_HEADER fh = nth.FileHeader;
		int sectionCnt;
		sectionCnt = fh.NumberOfSections;
		size_t number_of_section = (size_t)sectionCnt;
		IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[sectionCnt];
		get_IMAGE_SECTION_HEADERS(section_header, number_of_section, hF);
		if (fa < (dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + number_of_section * sizeof(IMAGE_SECTION_HEADER)))
			//fa还是在头部
		{
			return fa;
		}

		int sectionserial = 0;
		if (section_header != NULL) {
			for (sectionserial = 0; sectionserial < (number_of_section - 1); sectionserial++)
			{
				//cout<<section_header[i].VirtualAddress<<" "<<section_header[i+1].VirtualAddress<<endl;
				if (fa >= section_header[sectionserial].PointerToRawData && fa< section_header[sectionserial + 1].PointerToRawData)
					//夹在之间的,
				{
					break;
				}
			}
			int off = fa - section_header[sectionserial].PointerToRawData;
			rva= 4960 * sectionserial+4960+ off;
		}
		delete[] section_header;
		return rva;
	}


void get_IMAGE_IMPORT_DESCRIPTORS(IMAGE_IMPORT_DESCRIPTOR * rst, HANDLE hF, _IMAGE_NT_HEADERS64 nth, int i) {
	getBytes((char*)rst, sizeof(IMAGE_IMPORT_DESCRIPTOR) * i, rva_To_fa(nth.OptionalHeader.DataDirectory[1].VirtualAddress, hF), hF);
}

void get_IMAGE_IMPORT_DESCRIPTORS32(IMAGE_IMPORT_DESCRIPTOR * rst, HANDLE hF, _IMAGE_NT_HEADERS nth, int i) {
	//cout << rva_To_fa32(nth.OptionalHeader.DataDirectory[1].VirtualAddress,hF) << endl;
	getBytes((char*)rst, sizeof(IMAGE_IMPORT_DESCRIPTOR) * i, rva_To_fa32(nth.OptionalHeader.DataDirectory[1].VirtualAddress, hF), hF);
}

void PEchange(char * name) {
	HANDLE hF = CreateFileA(name, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (hF == INVALID_HANDLE_VALUE) {
		//std::cout <<NAME<< "未打开" << endl;
		return ;
	}
	char* IDH = new char[sizeof(_IMAGE_DOS_HEADER)];
	get_IMAGE_DOS_HEADER(IDH, hF);
	_IMAGE_DOS_HEADER dsh = (IMAGE_DOS_HEADER) * ((IMAGE_DOS_HEADER*)IDH);
	delete[] IDH;
	char* INH = new char[sizeof(_IMAGE_NT_HEADERS64)];
	get_IMAGE_NT_HEADER(INH, hF, dsh.e_lfanew);
	_IMAGE_NT_HEADERS64 nth = (_IMAGE_NT_HEADERS64) * ((_IMAGE_NT_HEADERS64*)INH);;
	delete[] INH;
	int import_count = 0;
	if (nth.FileHeader.SizeOfOptionalHeader == 240) {
		filetype = 64;
	}
	else {
		filetype = 32;
		char* INH = new char[sizeof(_IMAGE_NT_HEADERS)];
		get_IMAGE_NT_HEADER32(INH, hF, dsh.e_lfanew);
		_IMAGE_NT_HEADERS nth = (_IMAGE_NT_HEADERS) * ((_IMAGE_NT_HEADERS*)INH);;
		delete[] INH;	
	}
	int sectionnum = nth.FileHeader.NumberOfSections;
	size_t number_of_section = (size_t)sectionnum;
	IMAGE_SECTION_HEADER* section_header = new IMAGE_SECTION_HEADER[sectionnum];
	get_IMAGE_SECTION_HEADERS(section_header, number_of_section, hF);
	DWORD finalOfSections_fva = section_header[sectionnum - 1].PointerToRawData + section_header[sectionnum - 1].Misc.VirtualSize;
	//插入补丁代码文件
	char name2[]="C:\\Users\\lmc\\Desktop\\1.txt";
	HANDLE hF2 = CreateFileA(name2, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	int scannum = GetFileSize(hF2, NULL);
	int scansign = 0;
	DWORD nNumberOfBytesRead;
	char* asmfile = new char[scannum];
	ReadFile(hF2, asmfile, scannum, &nNumberOfBytesRead, NULL);
	DWORD codeAddSize = scannum;//获取插入补丁大小
	//section_header[sectionnum].Characteristics;
	//修正插入段的大小
	int size = section_header[sectionnum-1].SizeOfRawData + nth.OptionalHeader.FileAlignment * ((section_header[sectionnum - 1].Misc.VirtualSize + codeAddSize) / nth.OptionalHeader.FileAlignment);
	int sizerva = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + sizeof(IMAGE_SECTION_HEADER) * (sectionnum - 1) + sizeof(section_header[sectionnum - 1].Name) + sizeof(section_header[sectionnum - 1].Misc) + sizeof(section_header[sectionnum - 1].VirtualAddress);
	DWORD nNumberOfByteswrite;
	char *sizedst = new char[4];
	*sizedst= (char)size;
	
	if (size < 256) {
		SetFilePointer(hF, sizerva, NULL, FILE_BEGIN);
		WriteFile(hF, sizedst, sizeof(size), &nNumberOfByteswrite, NULL);
	}
	else if (size < 256 * 256) {
		*sizedst= (char)(size/256);
		sizerva ++;
		SetFilePointer(hF, sizerva, NULL, FILE_BEGIN);
		WriteFile(hF, sizedst, sizeof(size), &nNumberOfByteswrite, NULL);
	}
	else if (size < 256 * 256*256) {
		*sizedst= (char)(size / (256*256));
		sizerva+=2;
		SetFilePointer(hF, sizerva, NULL, FILE_BEGIN);
		WriteFile(hF, sizedst, sizeof(size), &nNumberOfByteswrite, NULL);
	}else {
		*sizedst= (char)(size / (256 * 256*256));
		sizerva+=3;
		SetFilePointer(hF, sizerva, NULL, FILE_BEGIN);
		WriteFile(hF, sizedst, sizeof(size), &nNumberOfByteswrite, NULL);
	}
	//修改msic
	int msic = section_header[sectionnum - 1].Misc.VirtualSize + codeAddSize;
	int msicrva = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + sizeof(IMAGE_SECTION_HEADER) * (sectionnum - 1) + sizeof(section_header[sectionnum - 1].Name);
	char* msicdst = new char[4];
	*msicdst = (char)msic;
	SetFilePointer(hF, msicrva, NULL, FILE_BEGIN);
	WriteFile(hF, msicdst, sizeof(msicdst), &nNumberOfByteswrite, NULL);
	//修改段属性
	section_header[sectionnum - 1].Characteristics= 0xE00000E0;
	int chararva = dsh.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + sizeof(IMAGE_SECTION_HEADER) * (sectionnum)-sizeof(section_header[sectionnum - 1].Characteristics);
	SetFilePointer(hF, chararva, NULL, FILE_BEGIN);
	WriteFile(hF, &section_header[sectionnum - 1].Characteristics, sizeof(section_header[sectionnum - 1].Characteristics), &nNumberOfByteswrite, NULL);
	//修改入口
	nth.OptionalHeader.AddressOfEntryPoint= fa_To_rva( section_header[sectionnum - 1].PointerToRawData+ section_header[sectionnum - 1].Misc.VirtualSize,hF);
	SetFilePointer(hF, dsh.e_lfanew+40, NULL, FILE_BEGIN);
	WriteFile(hF, &nth.OptionalHeader.AddressOfEntryPoint, sizeof(nth.OptionalHeader.AddressOfEntryPoint), &nNumberOfByteswrite, NULL);
	//写入补丁
	SetFilePointer(hF, section_header[sectionnum - 1].PointerToRawData + section_header[sectionnum - 1].Misc.VirtualSize, NULL, FILE_BEGIN);
	WriteFile(hF, asmfile,scannum, &nNumberOfByteswrite, NULL);
	//修改文件大小
	nth.OptionalHeader.SizeOfImage+=(1 + (codeAddSize / nth.OptionalHeader.SectionAlignment))*nth.OptionalHeader.SectionAlignment;
	SetFilePointer(hF, dsh.e_lfanew + 80, NULL, FILE_BEGIN);
	WriteFile(hF, &nth.OptionalHeader.SizeOfImage, sizeof(nth.OptionalHeader.SizeOfImage), &nNumberOfByteswrite, NULL);
	delete[] section_header;
	delete[] asmfile;
	delete[] sizedst;
	delete[] msicdst;
}

int main(int argc,char* argv[])
{
	char* Name = new char[260];
	setlocale(LC_ALL, "");
	if (argc > 0) {
		strcpy_s(Name, MAX_PATH, argv[1]);
	}
	PEchange(Name);
	delete[] Name;
}
