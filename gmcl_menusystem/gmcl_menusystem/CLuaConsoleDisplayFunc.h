class CLuaConsoleDisplayFunc : public IConsoleDisplayFunc
{
public:
	virtual void ColorPrint( const Color& clr, const char *pMessage );
	virtual void Print( const char *pMessage );
	virtual void DPrint( const char *pMessage );
};