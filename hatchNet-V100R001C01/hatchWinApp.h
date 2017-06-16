#ifndef HATCHWINAPP_H_H
#define HATCHWINAPP_H_H

class CHatchServices;
class CHatchThreadSimplePool;
class CHatchNetManager;

class CHatchApp
{
public:
	CHatchApp();
	virtual ~CHatchApp();

	int hatchMain(int argc, char *argv[]);

	virtual int hatchInitialize(void);
	virtual int hatchDetroy(void);

	int hatchInitializeLog(void);
	int hatchInitializeThreadPool(void);
	int hatchInitializeServices(void);
	int hatchInitializeNetLayer(void);
	int hatchInitializeRunner(void); 

	int hatchInitializeGlobalVar(void);
protected:
private:
	CHatchThreadSimplePool *m_appThreadPool;
	CHatchServices *m_appServices;
	CHatchNetManager *m_appNetManager;
};

extern CHatchApp *g_pHatchApp;

#endif