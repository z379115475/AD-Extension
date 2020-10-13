#include "stdafx.h"
#include "Kernel.h"
#include "mhook-lib/mhook.h"
#include "CxLog.h"

#define FIND_KEY(str,strKey) (str.Find(strKey) >= 0)
extern int g_nInterval;
extern CHAR g_strBkurl[256];

ATL::CSimpleArray<CModifier*> CKernel::m_SocketList;
funcNtDeviceIoControlFile CKernel::m_OrgNtDICtrlFile = NULL;
_ggShowConfig CKernel::m_ggShowConfig;

void CKernel::LoadUserConfig()
{
	m_ggShowConfig.nInterval = g_nInterval;
	SplitString(g_strBkurl, TEXT("@"), m_ggShowConfig.vBlackUrlList);
}

BOOL CKernel::Init()
{
	CUnGZip::Init();
	m_OrgNtDICtrlFile = (funcNtDeviceIoControlFile)GetProcAddress(LoadLibrary(TEXT("ntdll")), "NtDeviceIoControlFile");

	LoadUserConfig();
	return 0;
}

BOOL CKernel::RemoveUselessRequest(CString& strHead)
{
	int nBegin = strHead.Find("Host");
	if (nBegin < 0) {
		return TRUE;
	}
	int nEnd = strHead.Find("\r\n", nBegin);
	CString str = strHead.Mid(0, nEnd);
	if (FIND_KEY(str, ".png") || FIND_KEY(str, ".css") || FIND_KEY(str, ".js") || FIND_KEY(str, ".gif")
		|| FIND_KEY(str, ".jpg") || FIND_KEY(str, ".ico") || FIND_KEY(str, ".swf")) {
		return TRUE;
	}

	//排除黑名单
	for (size_t i = 0; i < m_ggShowConfig.vBlackUrlList.size(); i++) {
		if (FIND_KEY(str, m_ggShowConfig.vBlackUrlList[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

BOOL CKernel::RemoveRequestedUrl(CString& strHead)
{
	CString strHost = "";
	int nBegin;
	int nEnd;
	nBegin = strHead.Find("Host");
	if (nBegin >= 0) {
		nBegin += 6;
		nEnd = strHead.Find("\r\n", nBegin);
		strHost = strHead.Mid(nBegin, nEnd - nBegin);
	}

	CString strReferer = "";
	nBegin = strHead.Find("Referer");
	if (nBegin >= 0) {
		nBegin += 9;
		nEnd = strHead.Find("\r\n", nBegin);
		strReferer = strHead.Mid(nBegin, nEnd - nBegin);
	}

	for (size_t i = 0; i < m_ggShowConfig.vLastUrls.size(); i++) {
		if (FIND_KEY(strHost, m_ggShowConfig.vLastUrls[i]) || FIND_KEY(strReferer, m_ggShowConfig.vLastUrls[i])) {
			return TRUE;
		}
	}

	nBegin = strHost.Find(".");
	nBegin += 1;
	nEnd = strHost.Find(".", nBegin);
	CString strLastUrl = strHost.Mid(nBegin, nEnd - nBegin);	//www.huya.com -> huya
	if (strLastUrl.GetLength() > 1) {
		if (m_ggShowConfig.vLastUrls.size() == 20) {
			m_ggShowConfig.vLastUrls.erase(m_ggShowConfig.vLastUrls.begin());
		}
		m_ggShowConfig.vLastUrls.push_back(strLastUrl);
	}

	return FALSE;
}

BOOL CKernel::CheackData(CHAR* pBuf, LONG len)
{
	CString strHead = pBuf;
	if (!FIND_KEY(strHead, "GET /") || !FIND_KEY(strHead, "HTTP/1.1")) {
		return FALSE;
	}

	//广告插入时间间隔内
	DWORD dTime = GetTickCount();
	if (g_nLastTime != 0 && ((dTime / 1000 - g_nLastTime) < m_ggShowConfig.nInterval)) {
		if (!m_ggShowConfig.vLastUrls.empty()) {
			m_ggShowConfig.vLastUrls.clear();
		}
		return FALSE;
	}

	//排除图片、文件等请求,排除黑名单
	if (RemoveUselessRequest(strHead)) {
		return FALSE;
	}

	/*INT nBegin = strHead.Find("Host");
	INT nEnd = strHead.Find("\r\n", nBegin);
	CString str = strHead.Mid(0, nEnd);
	OutputDebugString("\n=========Request Url:=========\n");
	OutputDebugString(str);
	OutputDebugString("\n=======end=========\n");*/
	
	//排除掉同一网站中已经请求过的url，过滤多余的请求
	if (RemoveRequestedUrl(strHead)) {
		return FALSE;
	}

	return TRUE;
}

BOOL CKernel::AddModifier(HANDLE hFile)
{
	m_SocketList.Add(new CModifier(hFile,&m_ggShowConfig));

	return TRUE;
}

//*******************************目前JS插入失败的几种主要情况******************************************
//1.由于每个网站打开时发出的请求都比较多，目前是劫持网站发出的第一个请求进行处理，大多数情况第一个请求就是要处理的数据，偶尔会发生第一个是请求图片、文件之
//类的其他请求，我们需要的请求在后面才发出，所以就无法劫持到正确的请求
//2.有时可能会因为网络或者socket传输中发生问题，暂时用Sleep(1000)缓解此情况，但是偶尔还是会发生数据接收不正常导致JS无法插入（此情况的反应是正常运行JS没插入，
//但是打断点调试就插入成功了）
//***********插入失败只会无法显示广告，但是不影响网页的正常打开，也不会导致程序崩溃********************

NTSTATUS CKernel::RecvHandle(
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
	)
{
	NTSTATUS ret = 0;
	CModifier* pCurModifier = NULL;
	INT index = m_SocketList.GetSize() - 1;
	for (; index >= 0; index--) {
		if (FileHandle == m_SocketList[index]->m_hSocket) {
			pCurModifier = m_SocketList[index];
			break;
		}
	}

	if (pCurModifier) {
		switch (pCurModifier->m_Status) {
		case INIT_STATUS:
			// 等待第一个数据包，要设置编码格式
			Sleep(1000);
			// 获取网络数据
			ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
			if (InputBuffer) {
				AFD_INFO* ri = (AFD_INFO*)InputBuffer;
				if (ri->BufferArray->buf) {
					// 分析第一个数据包, 返回FALSE,删除这个socket，不在监视
					if (!pCurModifier->ParseFirstDataPacket(ri->BufferArray, IoStatusBlock, ret)) {
						delete pCurModifier;
						m_SocketList.RemoveAt(index);
					}
				}
			}
			break;
		case TRANS_STATUS:
			if (InputBuffer) {
				AFD_INFO* ri = (AFD_INFO*)InputBuffer;
				if (ri->BufferArray->len < 10) {
					//***此处(出现概率极低)，直接取消监听，网页会显示为空白，需刷新下，如果不处理会导致程序崩溃***
					LOG(TEXT("===========================ri->BufferArray->len < 10=====================\n"));
					if (pCurModifier->m_bIsInsert) {
						g_nLastTime = 0;
					}
					delete pCurModifier;
					m_SocketList.RemoveAt(index);
					break;
				}
				
				if (pCurModifier->m_In.IsEmpty()) {
					// 将浏览器buf链接到输入流
					pCurModifier->LinkToStream(ri->BufferArray, pCurModifier->m_In);
					ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
					if (ret < 0) {	
						if (IoStatusBlock->Information > 0 || !pCurModifier->m_In.IsEmpty() || !pCurModifier->m_Out.IsEmpty()) {
							//***此处为socket请求错误(出现概率极低)，直接取消监听，会闪过一个错误页面之后再显示正常页面，如果不处理会导致程序崩溃***
							LOG(TEXT("===========================ret < 0 && IoStatusBlock->Information > 0=====================\n"));
							if (pCurModifier->m_bIsInsert) {
								g_nLastTime = 0;
							}
							delete pCurModifier;
							m_SocketList.RemoveAt(index);
							break;
						}
						
					}
					// 断开浏览器buf与输入流的链接
					pCurModifier->BrekLinkFormStream(ri->BufferArray);
					if (pCurModifier->ParseTansDataPacket(IoStatusBlock->Information, ret)) {
						if (pCurModifier->m_In.m_isChunked) {
							pCurModifier->ReparseChunkData();
						}
					}
				}

				if (!pCurModifier->m_Out.IsEmpty()) {
					// 输出流中还有数据，直接将输出流中的数据返回
					pCurModifier->ReleaseData(ri->BufferArray, pCurModifier->m_Out, &(IoStatusBlock->Information));
					ret = 0;
				} else {
					if (pCurModifier->m_In.IsEmpty()) {
						if (pCurModifier->m_bIsInsert){		//发送数据上报消息
							if (g_hWnd) {
								::PostMessage(g_hWnd, WM_DATA_REPORT, NULL, NULL);
							}
						}

						memcpy(ri->BufferArray->buf, "\r\n0\r\n\r\n", 7);
						IoStatusBlock->Information = 7;
						ret = 0;	//-1073741661
						delete pCurModifier;
						m_SocketList.RemoveAt(index);
						break;
					}
					// 将输入流数据解压到输出流
					if (pCurModifier->StreamToStream(pCurModifier->m_In, pCurModifier->m_Out)) {
						pCurModifier->ReleaseData(ri->BufferArray, pCurModifier->m_Out, &(IoStatusBlock->Information));
					}
				}
			}
			break;
		default:
			ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
			break;
		}
	} else {
		ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}

	return ret;
}

NTSTATUS WINAPI CKernel::MyNtDICtrlFile(
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
	)
{
	NTSTATUS ret = 0;
	switch (IoControlCode) {
	case AFD_SEND:
		ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
		if (NT_SUCCESS(ret) && InputBuffer) {							// 发送数据劫持
			AFD_INFO* si = (AFD_INFO*)InputBuffer;
			if (CheackData(si->BufferArray->buf, si->BufferArray->len)) {
				AddModifier(FileHandle);
			}
		}
		break;

	case AFD_RECV:
		ret = RecvHandle(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
		break;
		

	default:
		ret = m_OrgNtDICtrlFile(FileHandle, Event, ApcRoutine, ApcContext, IoStatusBlock, IoControlCode, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength);
	}
	
	return ret;
}

VOID CKernel::SetHook()
{
	Mhook_SetHook((PVOID*)&CKernel::m_OrgNtDICtrlFile, CKernel::MyNtDICtrlFile);
}

VOID CKernel::UnHook()
{
	Mhook_Unhook((PVOID*)&CKernel::m_OrgNtDICtrlFile);
}