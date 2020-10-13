#include "stdafx.h"
#include "Utility.h"
#include <objbase.h>
#include <Shellapi.h>
#include <Shlwapi.h>
#include <wininet.h>
#include <time.h>

#pragma comment(lib,"Wininet.lib")

BOOL CreateHomeFolder(TString &strOutPath)
{
	TString strPath;
	TCHAR szTempPath[MAX_PATH] = {0};

	GetTempPath(MAX_PATH, szTempPath);
	strPath = szTempPath;

	GUID guid;
	CoCreateGuid(&guid);
	TCHAR szFolderName[MAX_PATH] = {0};

	_sntprintf_s(szFolderName, MAX_PATH, _TRUNCATE, TEXT("{%08lX-%04X-%04x-%02X%02X-%02X%02X%02X%02X%02X%02X}"),
		guid.Data1, 
		guid.Data2, 
		guid.Data3,
		guid.Data4[0], 
		guid.Data4[1], 
		guid.Data4[2], 
		guid.Data4[3],
		guid.Data4[4], 
		guid.Data4[5], 
		guid.Data4[6], 
		guid.Data4[7]);


	strPath += szFolderName;
	BOOL bRet = CreateDirectory(strPath, NULL);
	
	if (bRet) {
		strOutPath = strPath + TEXT("\\");
	}

	return TRUE;
}

BOOL GetKeyValue(TString szFileName, TString &szSection, TString szKeyName, TString &strValue)
{
	TCHAR szValue[MAX_PATH] = { 0 };
	BOOL bRet = FALSE;

	DWORD size = MAX_PATH;

	DWORD dwRet = GetPrivateProfileString(szSection, szKeyName, NULL, szValue, size, szFileName);
	if (dwRet == size - 1) {
		TCHAR* pBuf = new TCHAR[1024];
		size = 1024;
		dwRet = GetPrivateProfileString(szSection, szKeyName, NULL, pBuf, size, szFileName);
		if (dwRet == size - 1) {
			delete pBuf;
			return FALSE;
		}
		strValue = pBuf;
		delete pBuf;
	}
	else {
		strValue = szValue;
	}

	return (strValue.IsEmpty() ? FALSE : TRUE);
}

BOOL SetKeyValue(TString szFileName, TString &szSection, TString szKeyName, TString &strValue)
{
	return WritePrivateProfileString(szSection, szKeyName, strValue, szFileName);
}

BOOL ToLower(TString &strSrc)
{
	strSrc.MakeLower();

	return TRUE;
}

CHAR IntToHexChar(INT x)
{
	static const CHAR HEX[16] = {
		('0'), ('1'), ('2'), ('3'),
		('4'), ('5'), ('6'), ('7'),
		('8'), ('9'), ('A'), ('B'),
		('C'), ('D'), ('E'), ('F'),
	};

	return HEX[x];
}

INT HexCharToInt(CHAR hex)
{
	hex = toupper(hex);
	if (isdigit(hex))
		return (hex - '0');
	if (isalpha(hex))
		return (hex - 'A' + 10);

	return 0;
}


int key[8] = { 2, 0, 1, 5, 0, 6, 2, 3 };

BOOL Encrypt(LPCTSTR lpOrgStr, LPTSTR lpOutStr, INT len)
{
	LPSTR lpOut = new CHAR[(_tcslen(lpOrgStr) + 1) * 2];
	CObjRelease<CHAR> resout(lpOut);
	CT2A ansi(lpOrgStr);
	INT i = 0;
	LPSTR lpOrg = ansi;

	while (lpOrg[i]) {
		CHAR c = lpOrgStr[i] ^ key[i % 8];
		lpOut[i * 2] = IntToHexChar(c>>4);
		lpOut[i * 2 + 1] = IntToHexChar(c&0xF);
		i++;
	}

	lpOut[i*2] = '\0';
	CA2T t(lpOut);

	_tcscpy_s(lpOutStr, len, t);
	return TRUE;
}

BOOL Decryption(LPCTSTR lpOrgStr, LPTSTR lpOutStr, INT len)
{
	LPSTR lpOut = new CHAR[_tcslen(lpOrgStr) + 1];
	CObjRelease<CHAR> resout(lpOut);
	CT2A ansi(lpOrgStr);
	INT i = 0;
	INT index = 0;
	LPSTR lpOrg = ansi;

	while (lpOrg[i]) {
		CHAR c = (HexCharToInt(lpOrg[i]) << 4) | HexCharToInt(lpOrg[i+1]);
		lpOut[index] = c^ key[index % 8];
		i += 2;
		index++;
	}

	*(lpOut + index) = '\0';
	CA2T t(lpOut);

	_tcscpy_s(lpOutStr, len, t);

	return TRUE;
}

BOOL RemoveFolder(LPCTSTR lpszPath)
{
	TCHAR NewPath[MAX_PATH + 2] = { 0 };
	_tcscpy_s(NewPath, MAX_PATH + 2, lpszPath);

	SHFILEOPSTRUCT FileOp;
	ZeroMemory((void*)&FileOp,sizeof(SHFILEOPSTRUCT));
	
	FileOp.fFlags = FOF_SILENT| FOF_NOCONFIRMATION| FOF_NOERRORUI| FOF_NOCONFIRMMKDIR;
	FileOp.hNameMappings = NULL;
	FileOp.hwnd = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = NewPath;
	FileOp.pTo = NULL;
	FileOp.wFunc = FO_DELETE;

	return SHFileOperation(&FileOp) == 0;
}

BOOL Is64BitOpSystem()
{
	BOOL bRet = FALSE;
	SYSTEM_INFO si;    

	typedef VOID (WINAPI *LPFN_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);

	LPFN_GetNativeSystemInfo nsInfo = (LPFN_GetNativeSystemInfo)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");  
	if (NULL != nsInfo) {    
		nsInfo(&si);    
	} else {    
		GetSystemInfo(&si);    
	}    

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||    
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {    
			bRet = TRUE;    
	}

	return bRet;
}

BOOL SetPrivilege(HANDLE hProcess, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    HANDLE hToken;
    if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        return FALSE;
    }
    LUID luid;
    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
        return FALSE;
    }
    TOKEN_PRIVILEGES tkp;
    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = luid;
    tkp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : FALSE;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL)) {
        return FALSE;
    }
    return TRUE;
}

BOOL GetCurrentPath(TString& strCurPath, HMODULE hModu)
{
	TCHAR szFilePath[MAX_PATH + 1] = { 0 };
	
	GetModuleFileName(hModu, szFilePath, MAX_PATH);
	PathRemoveFileSpec(szFilePath);

	strCurPath = szFilePath;
	strCurPath += TEXT("\\");

	return TRUE;
}

BOOL SplitString(LPTSTR lpszSrc, LPTSTR lpszTag, vector<TString> &strArray)
{
	TCHAR *token;
	TCHAR *next_token = NULL;
	token = _tcstok_s(lpszSrc, lpszTag, &next_token);
	while (token) {
		strArray.push_back(token);
		token = _tcstok_s(NULL, lpszTag, &next_token);
	}

	return TRUE;
}

BOOL SplitString(TString &strSrc, LPTSTR lpszTag, vector<TString> &strArray)
{
	TCHAR szBuf[128] = { 0 };
	
	LPTSTR lpBuf = NULL;
	INT len = strSrc.GetLength()+1;
	if (len <= 128) {
		lpBuf = szBuf;
		len = 128;
	}
	else {
		lpBuf = new TCHAR[len];
		if (lpBuf == NULL) {
			return FALSE;
		}
	}

	_sntprintf_s(lpBuf, len, _TRUNCATE, strSrc);

	BOOL bRet = SplitString(lpBuf, lpszTag, strArray);
	if (len > 128) {
		delete lpBuf;
	}

	return bRet;
}

BOOL GetSubString(TString &strOring, LPCTSTR lpLeftTag, LPCTSTR lpRightTag, TString &strSub)
{
	INT leftPos = strOring.Find(lpLeftTag);
	if (leftPos == -1) {
		return FALSE;
	}

	leftPos += _tcslen(lpLeftTag);

	INT rightPos = strOring.Find(lpRightTag, leftPos);
	if (rightPos == -1) {
		strSub = strOring.Mid(leftPos);
	}
	else {
		strSub = strOring.Mid(leftPos, rightPos - leftPos);
	}

	return (strSub.IsEmpty() ? FALSE:TRUE);
}

BOOL GetKeyFormString(TString& lpContent, LPCTSTR lpKey, LPCTSTR lpTag, TString &strValue)
{
	TString strKey = lpKey;
	strKey += TEXT("=");
	return GetSubString(lpContent, strKey, lpTag, strValue);
}

BOOL ReNameDirectory(TString &pFrom, TString &pTo) 
{
	TCHAR form[MAX_PATH] = { 0 };
	TCHAR to[MAX_PATH] = { 0 };

	SHFILEOPSTRUCT  fo = { 0 };

	_sntprintf_s(form, MAX_PATH, _TRUNCATE, pFrom);
	_sntprintf_s(to, MAX_PATH, _TRUNCATE, pTo);

	fo.wFunc = FO_RENAME;
	fo.pFrom = form;
	fo.pTo = to;
	fo.fFlags = FOF_NOERRORUI /*| FOF_NOCONFIRMMKDIR*/ | FOF_NOCONFIRMATION;

	return (SHFileOperation(&fo) == 0);
}

BOOL GetTempPathString(TString &strTempPath)
{
	TCHAR szTempPath[MAX_PATH] = { 0 };
	GetTempPath(MAX_PATH, szTempPath);

	strTempPath = szTempPath;

	return TRUE;
}

BOOL GetSystemPathString(TString &strSystemPath)
{
	TCHAR szSystemPath[MAX_PATH] = { 0 };
	GetSystemDirectory(szSystemPath, MAX_PATH);

	strSystemPath = szSystemPath;
	strSystemPath += TEXT("\\");

	return TRUE;
}

BOOL IsXPAboveOS()
{
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);											// 获取当前操作系统的信息
	return ((osvi.dwMajorVersion > 5) ? TRUE : FALSE);				// xp以上系统
}

//CHAR* GetLocalIP()
//{
//	WSADATA wsaData;
//	CHAR name[255];
//	CHAR *ip = NULL;
//	PHOSTENT hostinfo;
//
//	if (WSAStartup(MAKEWORD(2,0), &wsaData) == 0) {
//		if (gethostname(name, sizeof(name)) == 0) {
//			if ((hostinfo = gethostbyname(name)) != NULL) {
//				ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
//			}
//		}
//		WSACleanup();
//	}
//
//	return ip;
//}

INT GetRadomNum(DWORD dwMax)
{
	static BOOL bIsInit = FALSE;

	if (!bIsInit) {
		srand((UINT)time(NULL));
		bIsInit = TRUE;
	}

	return (rand() % dwMax);
}

BOOL DeleteUrlCache()
{
	BOOL bRet = FALSE;
	HANDLE hEntry;
	LPINTERNET_CACHE_ENTRY_INFO lpCacheEntry = NULL;
	DWORD dwEntrySize = BUFFER_SIZE_1K;

	lpCacheEntry = (LPINTERNET_CACHE_ENTRY_INFO) new char[dwEntrySize];
	hEntry = FindFirstUrlCacheEntry(NULL, lpCacheEntry, &dwEntrySize);
	if (!hEntry) {
		return FALSE;
	}

	do {
		DeleteUrlCacheEntry(lpCacheEntry->lpszSourceUrlName);
		dwEntrySize = BUFFER_SIZE_1K;
	} while (FindNextUrlCacheEntry(hEntry, lpCacheEntry, &dwEntrySize));

	if (lpCacheEntry) {
		delete[] lpCacheEntry;
	}

	return TRUE;
}