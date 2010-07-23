#ifndef __CSTRING_H__
#define __CSTRING_H__

struct StringContainer
{
	int refCount;
	char *string;
	size_t stringLength;
};

class CString
{
public:
	CString(const char *str, size_t size)
	{
		m_pStrCont = new StringContainer();

		m_pStrCont->stringLength = size;

		m_pStrCont->string = (char *)malloc(size + 1);
		memcpy(m_pStrCont->string, str, size);

		// This is incase the string is used in a function that expects a nullterminated.
		// This is only a safeguard, nothing more.
		m_pStrCont->string[size] = 0x00;

		m_pStrCont->refCount = 1;
	};

	CString(const CString &other)
	{
		m_pStrCont = other.m_pStrCont;
		m_pStrCont->refCount++;
	};

	CString(void)
	{
		const char *str = "BAD_CSTRING";
		size_t size = strlen(str);

		m_pStrCont = new StringContainer();

		m_pStrCont->stringLength = size;

		m_pStrCont->string = (char *)malloc(size + 1);
		memcpy(m_pStrCont->string, str, size);

		// This is incase the string is used in a function that expects a nullterminated.
		// This is only a safeguard, nothing more.
		m_pStrCont->string[size] = 0x00;

		m_pStrCont->refCount = 1;
	};

	~CString(void)
	{
		m_pStrCont->refCount--;

		if(m_pStrCont->refCount <= 0)
		{
			free(m_pStrCont->string);
			delete m_pStrCont;
		}
	};

	const char *Str(size_t &out_Size)
	{
		out_Size = m_pStrCont->stringLength;
		return m_pStrCont->string;
	};

	const char *Str(void)
	{
		return m_pStrCont->string;
	};

	size_t Size(void)
	{
		return m_pStrCont->stringLength;
	};

	CString & operator=(const CString &other)
	{
		m_pStrCont->refCount--;

		if(m_pStrCont->refCount <= 0)
		{
			free(m_pStrCont->string);
			delete m_pStrCont;
		}

		m_pStrCont = other.m_pStrCont;
		m_pStrCont->refCount++;

		return *this;
	};

private:
	StringContainer *m_pStrCont;
};

#endif // __CSTRING_H__