#pragma once
#include <wininet.h>

// HTTP通信设置
#define HTTP_PROTOCOL	1
#define HTTP_PORT		(80)
#define HTTP_FLAGS		INTERNET_FLAG_NO_CACHE_WRITE

// HTTPS通信设置
#define HTTPS_PROTOCOL	2
#define HTTPS_PORT		(443)
#define HTTPS_FLAGS		(INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE | INTERNET_FLAG_IGNORE_CERT_CN_INVALID)

// 选择通信协议
#define SEL_PROTOCOL	HTTPS_PROTOCOL

#if SEL_PROTOCOL == HTTPS_PROTOCOL
#define PORT	HTTPS_PORT
#define FLAGS	HTTPS_FLAGS
#else
#define PORT	HTTP_PORT
#define FLAGS	HTTP_FLAGS
#endif

class CCloudRes
{
public:
	CCloudRes(LPCTSTR host, LPCTSTR url, INT port, LPCTSTR method, DWORD protocol = FLAGS) :
	m_hSession(NULL), m_hConnect(NULL), m_hRequest(NULL), m_isInitOk(FALSE)
	{
		Init(host, url, port, method, protocol);
	}

	~CCloudRes()
	{
		if (m_hRequest) InternetCloseHandle(m_hRequest);
		if (m_hConnect) InternetCloseHandle(m_hConnect);
		if (m_hSession) InternetCloseHandle(m_hSession);
	}

private:
	void Init(LPCTSTR host, LPCTSTR url, INT port, LPCTSTR method, DWORD protocol)
	{
		DWORD lAccessType = INTERNET_OPEN_TYPE_DIRECT;
		LPTSTR lpszProxyBypass = INTERNET_INVALID_PORT_NUMBER;
		m_hSession = InternetOpen(TEXT("Xt"), lAccessType, NULL, lpszProxyBypass, 0);
		if (m_hSession == NULL) {
			return;
		}

		m_hConnect = InternetConnect(m_hSession, host, port, TEXT(""), TEXT(""), INTERNET_SERVICE_HTTP, 0, 0);
		if (m_hConnect == NULL) {
			return;
		}

		m_hRequest = HttpOpenRequest(m_hConnect, method, url, NULL, NULL, NULL, protocol, 0);
		if (m_hRequest == NULL) {
			return;
		}

		m_isInitOk = TRUE;
	}
public:
	HINTERNET   m_hSession;
	HINTERNET   m_hConnect;
	HINTERNET	m_hRequest;
	BOOL		m_isInitOk;
};

class CCloudHelper
{
public:
	CCloudHelper()  {};
	~CCloudHelper() {};

	BOOL PostReq(TString& strUrl, TString& strBody, TString& strRetData, TString strHost = TONGJI_URL);
	BOOL GetReq(TString& strHost, TString& strUrl, TString& strRetData);
	BOOL FromUrlToFile(TString& strUrl, TString& strFileName);
	BOOL GetWANIP(TString& strIP);

private:
	BOOL CheckStatusInfo(HINTERNET hRequest);
	BOOL ReadReplyData(HINTERNET hRequest, TString& strResult);
};
