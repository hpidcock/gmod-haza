// Secret Headers extracted by haza55

abstract_class IMenuSystem001
{
public:
/*
Dumping IMenuSystem VTable:
	index=0 addr=1CB716A0 dlloffset=100016A0
	index=1 addr=1CB71B00 dlloffset=10001B00
	index=2 addr=1CB72190 dlloffset=10002190
	index=3 addr=1CB71BD0 dlloffset=10001BD0
	index=4 addr=1CB71FC0 dlloffset=10001FC0
	index=5 addr=1CB726C0 dlloffset=100026C0
	index=6 addr=1CB722F0 dlloffset=100022F0
	index=7 addr=1CB71BE0 dlloffset=10001BE0
	index=8 addr=1CB71DB0 dlloffset=10001DB0
*/

	virtual void* I0(const char *);
	virtual void* I1(void *, void *);
	virtual void* I2();
	virtual void* I3(void *);
	virtual bool Think();			// Call lua think hook.
	virtual bool PostRenderVGUI();	// Call lua postrendervgui hook.
	virtual void* I6(void *);
	virtual void* I7();
	virtual void* I8();
};