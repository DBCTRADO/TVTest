#ifndef EPG_DATA_LOADER_H
#define EPG_DATA_LOADER_H


#include "BonTsEngine/EventManager.h"


// EpgDataCap_BonのEPGデータを読み込むクラス
class CEpgDataLoader
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual ~CEventHandler() = 0;
		virtual void OnStart() {}
		virtual void OnEnd(bool fSuccess,CEventManager *pEventManager) {}
	};

	CEpgDataLoader();
	~CEpgDataLoader();
	bool Load(LPCTSTR pszFolder,HANDLE hAbortEvent=NULL);
	bool LoadAsync(LPCTSTR pszFolder,CEventHandler *pEventHandler=NULL);
	bool IsLoading() const;
	bool Abort(DWORD Timeout=10000);
	bool Wait(DWORD Timeout=INFINITE);

private:
	CEventManager m_EventManager;
	HANDLE m_hThread;
	HANDLE m_hAbortEvent;
	CDynamicString m_Folder;
	CEventHandler *m_pEventHandler;

	bool LoadFromFile(LPCTSTR pszFileName);
	static unsigned int __stdcall LoadThread(void *pParameter);
};


#endif	/* EPG_DATA_LOADER_H */
