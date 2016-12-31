#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H


#include <vector>
#include "ChannelList.h"


class CDriverInfo
{
public:
	CDriverInfo(LPCTSTR pszFileName);
	~CDriverInfo();
	LPCTSTR GetFileName() const { return m_FileName.c_str(); }
	LPCTSTR GetTunerName() const { return m_TunerName.c_str(); }
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
	TVTest::String m_FileName;
	TVTest::String m_TunerName;
	bool m_fChannelFileLoaded;
	CTuningSpaceList m_TuningSpaceList;
	bool m_fDriverSpaceLoaded;
	CTuningSpaceList m_DriverSpaceList;
};

class CDriverManager
{
public:
	struct TunerSpec
	{
		enum {
			FLAG_NETWORK       = 0x00000001U,
			FLAG_FILE          = 0x00000002U,
			FLAG_VIRTUAL       = 0x00000004U,
			FLAG_VOLATILE      = 0x00000008U,
			FLAG_NOENUMCHANNEL = 0x00000010U
		};

		unsigned int Flags;
	};

	CDriverManager();
	~CDriverManager();
	void Clear();
	bool Find(LPCTSTR pszDirectory);
	LPCTSTR GetBaseDirectory() const { return m_BaseDirectory.c_str(); }
	int NumDrivers() const { return (int)m_DriverList.size(); }
	CDriverInfo *GetDriverInfo(int Index);
	const CDriverInfo *GetDriverInfo(int Index) const;
	int FindByFileName(LPCTSTR pszFileName) const;
	bool GetAllServiceList(CChannelList *pList) const;

	bool LoadTunerSpec(LPCTSTR pszFileName);
	bool GetTunerSpec(LPCTSTR pszTunerName,TunerSpec *pSpec) const;

private:
	struct TunerSpecInfo
	{
		TVTest::String TunerMask;
		TunerSpec Spec;
	};

	std::vector<CDriverInfo*> m_DriverList;
	TVTest::String m_BaseDirectory;
	std::vector<TunerSpecInfo> m_TunerSpecList;
};


#endif
