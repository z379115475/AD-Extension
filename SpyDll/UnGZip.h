#pragma once

typedef LONG (__stdcall *pInitDecompression)();
typedef LONG (__stdcall *pCreateDecompression)(LONG *context, LONG flags);
typedef LONG (__stdcall *pDestroyDecompression)(LONG context);
typedef LONG (__stdcall *pDeInitDecompression)();
typedef LONG (__stdcall *pDecompress)(
									LONG context,
									const BYTE *in,
									LONG insize,
									BYTE *out,
									LONG outsize,
									LONG *inused,
									LONG *outused);

class CUnGZip
{
	static HMODULE m_hModu;
	
	static pInitDecompression InitDecompression;
	static pCreateDecompression CreateDecompression;
	static pDecompress Decompress;
	static pDestroyDecompression DestroyDecompression;
	static pDeInitDecompression DeInitDecompression;

	LONG m_ctx;
	
public:
	CUnGZip();
	~CUnGZip();
	static BOOL Init();
	BOOL xDecompress(BYTE* in, LONG insize, BYTE *out, LONG outsize, LONG *inused, LONG *outused);
};