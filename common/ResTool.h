#pragma once

class CHandleRelease
{
	HANDLE m_h;

public:
	CHandleRelease()
	{
		m_h = NULL;
	}

	CHandleRelease(HANDLE h)
	{
		m_h = h;
	}

	VOID Attach(HANDLE h)
	{
		m_h = h;
	}

	~CHandleRelease()
	{
		CloseHandle(m_h);
	}
};

template <class T>
class CObjRelease
{
	T *m_p;

public:
	CObjRelease()
	{
		m_p = NULL;
	}
	CObjRelease(T *p)
	{
		m_p = p;
	}

	VOID Attach(T *p)
	{
		m_p = p;
	}

	~CObjRelease()
	{
		if (m_p) {
			delete m_p;
		}
	}
};
