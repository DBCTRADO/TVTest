#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H


#include <vector>
#include "ChannelList.h"


class CDriverInfo
{
public:
	CDriverInfo(LPCTSTR pszFileName);
	~CDriverInfo();
	LPCTSTR GetFileName() const { return m_FileName.Get(); }
	LPCTSTR GetTunerName() const { return m_TunerName.Get(); }
	enum LoadTuningSpaceListMode {
		LOADTUNINGSPACE_DEFAULT,
		LOADTUNINGSPACE_NOLOADDRIVER,
		LOADTUNINGSPACE_USEDRIVER,
		LOADTUNINGSPACE_USEDRIVER_NOOPEN,
	};
	bool LoadTuningSpaceList(LoadTuningSpaceListMode Mode=LOADTUNINGSPACE_DEFAULT);
	void ClearTuningSpaceList();
	bool IsChannelFileLoaded() const { return m_fChannelFileLoaded; }
	bool IsDriverChannelLoaded() const { return m_fDriverSpaceLoaded; }
	bool IsTuningSpaceListLoaded() const {
		return m_fChannelFileLoaded || m_fDriverSpaceLoaded;
	}
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	const CTuningSpaceList *GetDriverSpaceList() const { return &m_DriverSpaceList; }
	const CTuningSpaceList *GetAvailableTuningSpaceList() const;
	const CChannelList *GetChannelList(int Space) const;
	int NumDriverSpaces() const { return m_DriverSpaceList.NumSpaces(); }

private:
	CDynamicString m_FileName;
	CDynamicString m_TunerName;
	bool m_fChannelFileLoaded;
	CTuningSpaceList m_TuningSpaceList;
	bool m_fDriverSpaceLoaded;
	CTuningSpaceList m_DriverSpaceList;
};

class CDriverManager
{
public:
	CDriverManager();
	~CDriverManager();
	void Clear();
	bool Find(LPCTSTR pszDirectory);
	LPCTSTR GetBaseDirectory() const { return m_BaseDirectory.Get(); }
	int NumDrivers() const { return (int)m_DriverList.size(); }
	CDriverInfo *GetDriverInfo(int Index);
	const CDriverInfo *GetDriverInfo(int Index) const;
	int FindByFileName(LPCTSTR pszFileName) const;
	bool GetAllServiceList(CChannelList *pList) const;

private:
	std::vector<CDriverInfo*> m_DriverList;
	CDynamicString m_BaseDirectory;
};


#endif
