#pragma once
#include <Winternl.h>
#include "UnGZip.h"
#include "Modifier.h"
#include <vector>

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

class CKernel
{
public:
	static ATL::CSimpleArray<CModifier*> m_SocketList;
	static funcNtDeviceIoControlFile m_OrgNtDICtrlFile;

	static _ggShowConfig m_ggShowConfig;

	static CKernel& GetInstance()
	{
		static CKernel obj;

		return obj;
	}

	static BOOL Init();
	static void LoadUserConfig();
	static NTSTATUS WINAPI MyNtDICtrlFile(
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

	static BOOL CheackData(CHAR* pBuf, LONG len);
	static BOOL RemoveRequestedUrl(CString& strHead);
	static BOOL RemoveUselessRequest(CString& strHead);
	static BOOL AddModifier(HANDLE hFile);
	static NTSTATUS RecvHandle(
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

	static VOID SetHook();
	static VOID UnHook();
};