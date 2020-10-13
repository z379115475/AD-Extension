#pragma once
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <process.h>
// C 运行时头文件
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <Shellapi.h>
#include <Shlwapi.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <atlbase.h>
#include <atlstr.h>

// TODO:  在此处引用程序需要的其他头文件
#include "ResTool.h"

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Shell32.lib")
#pragma comment(lib,"Shlwapi.lib")

using namespace std;

#define _S_OK(a) \
{ if (!a) return FALSE; }

#define _S(a) \
{ hr = (a); if (FAILED(hr)) return FALSE; }

#define disp_cast(disp) \
	((CComDispatchDriver&)(void(static_cast<IDispatch*>(disp)), reinterpret_cast<CComDispatchDriver&>(disp)))

#define TString ATL::CString

#define MD5SIG				TEXT("awangba.com")

#define PIPE_NAME			TEXT("\\\\.\\pipe\\wangbax")
#define PIPE_BUFFER_SIZE	1024

#define TONGJI_URL          TEXT("tongji.52wblm.com")       // 数据上报
#define DOMAIN_URL          TEXT("domain.52wblm.com")       // 配置文件


#define P2P_CLIENT_PACKLEN	32768		// 数据包长度
#define P2P_SERVER_PACKLEN	65536		// 数据包长度
#define P2P_PACKBUFFER_LEN	65536		// 数据包长度
#define MAX_CLIENT_COUNT    5			// 最大客户端连接数

#define BUFFER_SIZE_1K		1024		//1K缓冲区大小
#define BUFFER_SIZE_4K		4096		//4K缓冲区大小

typedef struct _t_IPINFO {
	TString ip;
	USHORT port;
}IPINFO;

typedef enum {
	UNDOWNLOAD = 0,			// 业务文件包未下载
	DOWNLOAD_OK,			// 业务文件包下载成功
	DOWNLOAD_NG,			// 业务文件包下载失败
	MD5CHECK_OK,			// MD5 Check成功
	MD5CHECK_NG,			// MD5 Check失败
	UNZIP_OK,				// 业务文件包解压成功
	UNZIP_NG,				// 业务文件包解压失败
	RUN_OK,					// 业务文件包运行成功
	RUN_NG,					// 业务文件包运行失败
	PLATFORM_NOMATCH		// 该业务不支持当前的平台
} FSTATUS;

// 文件信息
typedef struct _t_FILEINFO{
	TString filename;		// 文件名
	TString md5;			// MD5
	TString downaddr;		// 下载URL地址
	TString bootcmd;		// 引导命令
	TString platform;		// 平台
	TString localpath;		// 存放路径
	FSTATUS status;			// 下载状态
} FILEINFO;

// 业务信息

typedef struct _t_PROJECTINFO {
	FSTATUS		pjstatus;				// 业务状态
	TString		projname;				// 业务名
	FILEINFO	*fileinfo;				// 业务包
} PROJECTINFO;

typedef struct _t_USERCONFIGINFO{
	BOOL				isremitem;		// 是否有业务被删除
	DWORD				time;			// 上报数据定时器
	TString				userid;			// 用户id
	TString				usergid;		// 用户组id
	TString				username;		// 用户名
	TString				rootpath;		// 用户根目录
	TString				usertype;		// 用户类型
	TString				userexp;		// 用户扩展参数
	vector<FILEINFO>	projlist;		// 用户业务信息
} USERINFO;

typedef INT (*LogFunc)(const LPTSTR strLog);

BOOL CreateHomeFolder(TString &strOutPath);
BOOL GetKeyValue(TString szFileName, TString &szSection, TString szKeyName, TString &strValue);
BOOL SetKeyValue(TString szFileName, TString &szSection, TString szKeyName, TString &strValue);
BOOL GetUsername(TString& strUserName);
BOOL ToLower(TString &strSrc);
BOOL RemoveFolder(LPCTSTR lpszPath);
BOOL Is64BitOpSystem();
BOOL IsXPAboveOS();
BOOL SetPrivilege(HANDLE hProcess, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
BOOL GetCurrentPath(TString& strCurPath, HMODULE hModu = NULL);
BOOL GetCurrentPath(LPTSTR szCurPath, DWORD dwSize);
BOOL SplitString(LPTSTR lpszSrc, LPTSTR lpszTag, vector<TString> &strArray);
BOOL SplitString(TString &strSrc, LPTSTR lpszTag, vector<TString> &strArray);
BOOL ReNameDirectory(TString &pFrom, TString &pTo);
BOOL StringToKeyValue(TString& strSrc, map<TString, TString> &mapOutKeyVal);
BOOL GetTempPathString(TString &strTempPath);
BOOL GetSystemPathString(TString &strSystemPath);
BOOL GetSubString(TString &strOring, LPCTSTR lpLeftTag, LPCTSTR lpRightTag, TString &strSub);
BOOL GetKeyFormString(TString& lpContent, LPCTSTR lpKey, LPCTSTR lpTag, TString &strValue);
CHAR* GetLocalIP();
INT GetRadomNum(DWORD dwMax);
BOOL DeleteUrlCache();
BOOL Encrypt(LPCTSTR lpOrgStr, LPTSTR lpOutStr, INT len);
BOOL Decryption(LPCTSTR lpOrgStr, LPTSTR lpOutStr, INT len);