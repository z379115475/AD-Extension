// APlusMd5.h: interface for the CAPlusMd5 class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#include <tchar.h>
#include <wincrypt.h>

class CryptResRelease
{
	HCRYPTPROV *m_phProv;
	HCRYPTHASH *m_phHash;
public:

	CryptResRelease(HCRYPTPROV *phProv, HCRYPTHASH *phHash)
	{
		m_phProv = phProv;
		m_phHash = phHash;
	}

	~CryptResRelease()
	{
		if (m_phHash && *m_phHash) {
			CryptDestroyHash(*m_phHash);
		}

		if (m_phProv && *m_phProv) {
			CryptReleaseContext(*m_phProv, 0);
		}
	}
};

class CMd5  
{
public:

	static BOOL GetHash(CONST BYTE *pbData, DWORD dwDataLen, ALG_ID algId, LPTSTR pszHash)
	{
		HCRYPTPROV hProv = NULL;
		if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
			return FALSE;
		}

		HCRYPTHASH hHash = NULL;
		CryptResRelease res(&hProv, &hHash);
		//Alg Id:CALG_MD5,CALG_SHA
		if (!CryptCreateHash(hProv, algId, 0, 0, &hHash)) {
			return FALSE;
		}

		if (!CryptHashData(hHash, pbData, dwDataLen, 0)) {
			return FALSE;
		}

		DWORD dwSize;
		DWORD dwLen = sizeof(dwSize);
		CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)(&dwSize), &dwLen, 0);

		BYTE* pHash = new BYTE[dwSize];
		CObjRelease<BYTE> ResByte(pHash);
		dwLen = dwSize;
		CryptGetHashParam(hHash, HP_HASHVAL, pHash, &dwLen, 0);

		lstrcpy(pszHash, _T(""));
		TCHAR szTemp[3];
		for (DWORD i = 0; i < dwLen; ++i) {
			wsprintf(szTemp, _T("%02x"), pHash[i]);
			lstrcat(pszHash, szTemp);
		}

		return TRUE;
	}

	static BOOL GetFileMd5(LPCTSTR lpFileName, LPTSTR pszHash)
	{
		HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		CHandleRelease Res(hFile);
		if (hFile == INVALID_HANDLE_VALUE) {
			return FALSE;
		}

		DWORD dwFileSize = GetFileSize(hFile, 0);    //获取文件的大小
		if (dwFileSize == 0xFFFFFFFF){
			return FALSE;
		}

		BYTE* lpReadFileBuffer = new BYTE[dwFileSize];
		CObjRelease<BYTE> ResByte(lpReadFileBuffer);

		DWORD lpReadNumberOfBytes;
		if (!ReadFile(hFile, lpReadFileBuffer, dwFileSize, &lpReadNumberOfBytes, NULL)){
			return FALSE;
		}

		if (!GetHash(lpReadFileBuffer, dwFileSize, CALG_MD5, pszHash)) {
			return FALSE;
		}

		return TRUE;
	}

	static BOOL GetStringMd5(LPSTR pszStr, LPTSTR pszHash)
	{
		if (GetHash((BYTE*)pszStr, strlen(pszStr), CALG_MD5, pszHash)) {
			return FALSE;
		}

		return TRUE;
	}

	static TString GetSignature(LPCTSTR szCustomData)
	{
		TString strSingnature;

		SYSTEMTIME tt;

		GetLocalTime(&tt);
		strSingnature.Format(TEXT("%4d-%02d-%02d%s"), tt.wYear, tt.wMonth, tt.wDay, szCustomData);

		TCHAR szMd5[33] = { 0 };
		GetStringMd5(CT2A(strSingnature), szMd5);

		return szMd5;
	}

};

