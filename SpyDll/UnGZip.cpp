#include "stdafx.h"
#include "UnGZip.h"

HMODULE CUnGZip::m_hModu = NULL;

pDecompress CUnGZip::Decompress = NULL;
pInitDecompression CUnGZip::InitDecompression = NULL;
pDeInitDecompression CUnGZip::DeInitDecompression = NULL;
pCreateDecompression CUnGZip::CreateDecompression = NULL;
pDestroyDecompression CUnGZip::DestroyDecompression = NULL;

CUnGZip::CUnGZip()
{
	const LONG GZIP_LVL = 1;

	InitDecompression();
    CreateDecompression(&m_ctx, GZIP_LVL);
}

CUnGZip::~CUnGZip()
{
	DestroyDecompression(m_ctx);
	DeInitDecompression();
}

BOOL CUnGZip::Init()
{
	BOOL bRet = FALSE;
	m_hModu = LoadLibrary(TEXT("C:\\windows\\system32\\gzip.dll"));

	if (m_hModu) {
		InitDecompression = (pInitDecompression)GetProcAddress(m_hModu, "InitDecompression");
		CreateDecompression = (pCreateDecompression)GetProcAddress(m_hModu, "CreateDecompression");
		Decompress = (pDecompress)GetProcAddress(m_hModu, "Decompress");
		DestroyDecompression = (pDestroyDecompression)GetProcAddress(m_hModu, "DestroyDecompression");
		DeInitDecompression = (pDeInitDecompression)GetProcAddress(m_hModu, "DeInitDecompression");
	}

	if (Decompress == NULL) {
		bRet = TRUE;
	}

	return bRet;
}


BOOL CUnGZip::xDecompress(BYTE* in, LONG insize, BYTE *out, LONG outsize, LONG *inused, LONG *outused)
{
    if (m_ctx) {
		*inused = 0, *outused = 0;
		Decompress(m_ctx, in, insize, out, outsize, inused, outused);
    }

	return TRUE;
}