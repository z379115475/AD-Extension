/*
 * @文  件  名：CloudHelper.cpp
 * @说      明：CCloudHelper实现
 * @日      期：Created on: 2014/03/03
 * @版      权：Copyright 2014
 * @作      者：MCA
 */

#include "StdAfx.h"
#include <Urlmon.h>
#include "Utility.h"
#include "CloudHelper.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "Urlmon.lib")

/**
* \brief  发送Post请求
* \return 获取成功否
*/
BOOL CCloudHelper::PostReq(TString& strUrl, TString& strBody, TString& strRetData, TString strHost)
{
	CCloudRes res(strHost, strUrl, PORT, TEXT("POST"));
	if (!res.m_isInitOk) {
		return FALSE;
	}

	BOOL bRet = HttpAddRequestHeaders(res.m_hRequest,
		TEXT("Connection: keep-alive\r\nContent-Type: application/x-www-form-urlencoded\r\n"),
		-1,
		HTTP_ADDREQ_FLAG_ADD);
	if (!bRet) {
		return FALSE;
	}

	CT2A lszBody(strBody);

	bRet = HttpSendRequest(res.m_hRequest, NULL, 0, lszBody, strBody.GetLength());
	if (!bRet && (GetLastError() == ERROR_INTERNET_INVALID_CA)) {
		DWORD dwFlags;
		DWORD dwBuffLen = sizeof(dwFlags);   
		InternetQueryOption(res.m_hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&dwFlags, &dwBuffLen);
		dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;   
		InternetSetOption(res.m_hRequest, INTERNET_OPTION_SECURITY_FLAGS, &dwFlags, sizeof (dwFlags));
	
		bRet = HttpSendRequest(res.m_hRequest, NULL, 0, lszBody, strBody.GetLength());
	}

	if (!bRet) {
		return FALSE;
	}

	if (!CheckStatusInfo(res.m_hRequest)) {
		return FALSE;
	}

	return ReadReplyData(res.m_hRequest, strRetData);
}

 /**
  * \brief  从指定的URL下载文件到本地
  * \param  strUrl: 
  * \return 
  */
BOOL CCloudHelper::FromUrlToFile(TString& strUrl, TString& strFileName)
{
	HRESULT hResult = URLDownloadToFile(NULL, strUrl, strFileName, 0, NULL);

	return (hResult == S_OK) ? TRUE : FALSE;
}

/**
* \brief  发送Get请求
* \return 获取成功否
*/
BOOL CCloudHelper::GetReq(TString& strHost, TString& strUrl, TString& strRetData)
{
	CCloudRes res(strHost, strUrl, HTTP_PORT, TEXT("GET"), HTTP_FLAGS);
	if (!res.m_isInitOk) {
		return FALSE;
	}

	if (!HttpSendRequest(res.m_hRequest, NULL, 0, NULL, 0)) {
		return FALSE;
	}

	if (!CheckStatusInfo(res.m_hRequest)) {
		return FALSE;
	}

	return ReadReplyData(res.m_hRequest, strRetData);
}

BOOL CCloudHelper::GetWANIP(TString& strIP)
{
	//LOG(TEXT("GetWANIP start"));
	//TString strHost = TEXT("www.3322.org");
	//TString strUrl  = TEXT("/dyndns/getip");
	//BOOL bRet = GetReq(strHost, strUrl, strIP);
	//if (bRet) {
	//	LOG(TEXT("GetReq1 ok"));
	//	strIP.Remove(0x0a);		// 删除0x0a
	//	return bRet;
	//}
	//LOG(TEXT("GetReq1 fail"));
	TString strHost = TONGJI_URL;
	TString strUrl = TEXT("/client/ip");

	return GetReq(strHost, strUrl, strIP);
}

 /**
  * \brief  获取响应信息
  * \param  hResult: 请求连接
  * \return 获取的返回值
  */
BOOL CCloudHelper::ReadReplyData(HINTERNET hRequest, TString& strResult)
{
	BOOL  bRet = FALSE;
	DWORD dwBytesRead = 0;
	DWORD dwTotalBytes = 0;
	DWORD dwBytesAvailable = 0;

	CHAR  *pAnsiBuf = new CHAR[BUFFER_SIZE_4K];
	if (pAnsiBuf == NULL) {
		return FALSE;
	}
	CObjRelease<CHAR> ResChar(pAnsiBuf);

	ZeroMemory(pAnsiBuf, BUFFER_SIZE_4K);

	CHAR* pBuff = pAnsiBuf;

	while (InternetQueryDataAvailable(hRequest, &dwBytesAvailable, 0, 0)) {
		if (dwBytesAvailable == 0) {
			bRet = TRUE;
			break;
		}

		if (dwTotalBytes + dwBytesAvailable >= BUFFER_SIZE_4K) {
			bRet = FALSE;
			break;
		}
		
		if (!InternetReadFile(hRequest, pBuff + dwTotalBytes, dwBytesAvailable, &dwBytesRead)) {
			bRet = FALSE;
			break;
		}
		dwTotalBytes += dwBytesRead;
	}
	if (bRet) {
		strResult = CA2T(pAnsiBuf);
	}

	return bRet;
}

 /**
 * \brief  检查返回状态
 * \param  hResult: 请求
 * \return 请求是否成功
 */
BOOL CCloudHelper::CheckStatusInfo(HINTERNET hRequest)
{	
	DWORD dwStatusCode; 
	DWORD dwSize = sizeof(DWORD); 

	BOOL bRet = HttpQueryInfo(hRequest, 
		HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
		&dwStatusCode, 
		&dwSize, 
		NULL
	);

	if ((!bRet) || (dwStatusCode != 200)) {
		return FALSE;
	}

	return TRUE;
}