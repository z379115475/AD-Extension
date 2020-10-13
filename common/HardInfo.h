#pragma once
#include "Utility.h"

class CHardInfo
{
public:
	CHardInfo();
	~CHardInfo();

public:
	TString GetMac();
	BOOL GetMac1(TString &mac);
	BOOL GetMac2(TString &mac);
	TString GetSysInfo();

private:
	VOID SafeGetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo);

private:
	static TString m_strGraphicsInfo;
	static TString m_strOSVersion;
	static TString m_strMAC;
	static DWORD   m_dwCPUCores;
	static DWORD   m_dwCPUThreads;
};

