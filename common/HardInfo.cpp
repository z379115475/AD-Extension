#include "StdAfx.h"
#include "HardInfo.h"
#include "md5.h"
#include "Utility.h"
#include <iphlpapi.h>

#pragma comment(lib,"iphlpapi.lib")


#define OID_802_3_PERMANENT_ADDRESS      0x01010101
#define IOCTL_NDIS_QUERY_GLOBAL_STATS    0x00170002

TString CHardInfo::m_strGraphicsInfo = TEXT("");
TString CHardInfo::m_strOSVersion = TEXT("");
TString CHardInfo::m_strMAC = TEXT("");
DWORD   CHardInfo::m_dwCPUCores = 0;
DWORD   CHardInfo::m_dwCPUThreads = 0;

CHardInfo::CHardInfo()
{
}


CHardInfo::~CHardInfo()
{
}   


 /**
  * \brief  获取MAC地址
  * \return mac地址
  */
TString CHardInfo::GetMac()
{
	if (!m_strMAC.IsEmpty()) {
		return m_strMAC;
	}

	m_strMAC = TEXT("00:00:00:00:00:00");
	BOOL bRet = GetMac1(m_strMAC);
	if (!bRet) {
		GetMac2(m_strMAC);
	}

	return m_strMAC;
}

/**
* \brief  获取MAC地址
* \return mac地址
*/
BOOL CHardInfo::GetMac2(TString &strMac)
{
	BOOL bRet = FALSE;
	DWORD dwOutBufLen = sizeof(IP_ADAPTER_INFO);
	PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO *) new CHAR[dwOutBufLen];
	if (pAdapterInfo == NULL) {
		return FALSE;
	}

	if (GetAdaptersInfo(pAdapterInfo, &dwOutBufLen) == ERROR_BUFFER_OVERFLOW) {
		delete[] pAdapterInfo;
		pAdapterInfo = (IP_ADAPTER_INFO *) new CHAR[dwOutBufLen];
		if (pAdapterInfo == NULL) {
			return FALSE;
		}
	}

	if (GetAdaptersInfo(pAdapterInfo, &dwOutBufLen) == NO_ERROR) {
		BYTE s[8] = { 0 };
		TCHAR mac[32] = { 0 };
		memcpy(s, pAdapterInfo->Address, 6);
		_sntprintf_s(mac,
			32,
			_TRUNCATE,
			TEXT("%02X:%02X:%02X:%02X:%02X:%02X"),
			s[0], s[1], s[2], s[3], s[4], s[5]
			);
		strMac = mac;
		bRet = TRUE;
	}
	delete[] pAdapterInfo;

	return bRet;
}

/**
* \brief  获取MAC地址
* \return mac地址
*/
BOOL CHardInfo::GetMac1(TString &strMac)
{
	BOOL bRet = FALSE;
	HKEY hKey = NULL;
	HKEY hKey2 = NULL;
	TCHAR szMac[MAX_PATH] = { 0 };
	TCHAR szKey[MAX_PATH] = { 0 };
	TCHAR szBuffer[MAX_PATH] = { 0 };
	TCHAR szServiceName[MAX_PATH] = { 0 };
	TCHAR szFileName[MAX_PATH] = { 0 };
	DWORD dwRet = 0;
	DWORD dwType = 0;
	DWORD cbData = 0;
	DWORD cName = _countof(szBuffer);

	if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\"), &hKey) != ERROR_SUCCESS){
		return FALSE;
	}

	for (int i = 0; RegEnumKeyEx(hKey, i, szBuffer, &cName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS; ++i, cName = _countof(szBuffer)){
		lstrcpy(szKey, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\"));
		lstrcat(szKey, szBuffer);
		if (RegOpenKey(HKEY_LOCAL_MACHINE, szKey, &hKey2) != ERROR_SUCCESS){
			continue;
		}

		dwType = REG_SZ;
		cbData = MAX_PATH * sizeof(CHAR);
		if (RegQueryValueEx(hKey2, TEXT("ServiceName"), NULL, &dwType, (LPBYTE)szServiceName, &cbData) == ERROR_SUCCESS){
			RegCloseKey(hKey2);

			lstrcpy(szFileName, TEXT("\\\\.\\"));
			lstrcat(szFileName, szServiceName);
			HANDLE hFile = CreateFile(szFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE){
				DWORD dwInBuff = OID_802_3_PERMANENT_ADDRESS;
				BYTE outBuff[MAX_PATH];
				dwRet = DeviceIoControl(hFile, IOCTL_NDIS_QUERY_GLOBAL_STATS, &dwInBuff, sizeof(dwInBuff), outBuff, sizeof(outBuff), &cbData, NULL);

				CloseHandle(hFile);
				hFile = INVALID_HANDLE_VALUE;

				if (dwRet){
					_sntprintf_s(szMac,
						MAX_PATH,
						_TRUNCATE,
						TEXT("%02X:%02X:%02X:%02X:%02X:%02X"),
						outBuff[0],
						outBuff[1],
						outBuff[2],
						outBuff[3],
						outBuff[4],
						outBuff[5]
						);
					strMac = szMac;
					bRet = TRUE;
					break;
				}
			}
		}
		else {
			RegCloseKey(hKey2);
		}
	}

	if (hKey != NULL){
		RegCloseKey(hKey);
	}

	return bRet;
}

/**
 * \brief  获取操作系统版本
 * \return 操作系统版本
 */
TString CHardInfo::GetSysInfo()
{
	if (!m_strOSVersion.IsEmpty()) {
		return m_strOSVersion;
	}

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    OSVERSIONINFOEX os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if (GetVersionEx((OSVERSIONINFO*)&os)) {         
        switch (os.dwMajorVersion){               
            case 4:
                switch(os.dwMinorVersion){  
                    case 0:
                        if(os.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                            m_strOSVersion = TEXT("WINT4.0");
						} else if (os.dwPlatformId==VER_PLATFORM_WIN32_WINDOWS) {
                            m_strOSVersion = TEXT("Win95");
						}
                        break;
                    case 10:
                        m_strOSVersion = TEXT("Win98");
                        break;
                    case 90:
                        m_strOSVersion = TEXT("Winme");
                        break;
                }
                break;
            case 5:
                switch(os.dwMinorVersion){
                    case 0:
                        m_strOSVersion = TEXT("Win2000");
                        break;
                    case 1:
                        m_strOSVersion = TEXT("winxp");
                        break;
                    case 2:
                        if(os.wProductType == VER_NT_WORKSTATION && info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
                            m_strOSVersion = TEXT("Microsoft Windows XP Professional x64 Edition");
						} else if(GetSystemMetrics(SM_SERVERR2) == 0) {
                            m_strOSVersion = TEXT("Microsoft Windows Server 2003"); 
						} else if(GetSystemMetrics(SM_SERVERR2)!=0) {
                            m_strOSVersion =TEXT( "Microsoft Windows Server 2003 R2");
						}
                        break;
                }
                break;
            case 6:
                switch(os.dwMinorVersion){
                    case 0:
                        if(os.wProductType == VER_NT_WORKSTATION)  {
                            m_strOSVersion = TEXT("vista");
						} else {
                            m_strOSVersion = TEXT("WinServer2008");
						}
                        break;
                    case 1:
                        if(os.wProductType == VER_NT_WORKSTATION) {
                            m_strOSVersion = TEXT("win7");
						} else {
                            m_strOSVersion = TEXT("WinServer2008R2");
						}
                        break;
					case 2:
						m_strOSVersion = TEXT("win8");
						break;
                }
                break;
            default:
                m_strOSVersion = TEXT("Unknown");
        }
  
    } 
	if (Is64BitOpSystem()) {
		m_strOSVersion += TEXT("_x64");
	}
	return m_strOSVersion;
}
