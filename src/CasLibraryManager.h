#ifndef CAS_LIBRARY_MANAGER_H
#define CAS_LIBRARY_MANAGER_H


#include <vector>
#include "Settings.h"


class CCasLibraryManager
{
public:
	bool LoadSettings(CSettings &Settings);
	void SetDefaultCasLibrary(LPCTSTR pszFileName);
	LPCTSTR GetDefaultCasLibrary() const { return m_DefaultCasLibrary.c_str(); }
	bool HasDefaultCasLibrary() const { return !m_DefaultCasLibrary.empty(); }
	bool HasCasLibraryNetworkMap() const { return !m_CasLibraryNetworkMap.empty(); }
	bool FindCasLibraries(LPCTSTR pszDirectory,std::vector<TVTest::String> *pList) const;
	bool FindDefaultCasLibrary(LPCTSTR pszDirectory);
	const TVTest::String &GetCasLibraryFileName(WORD NetworkID,WORD TSID) const;
	const TVTest::String &GetCasDeviceNameByTunerName(LPCTSTR pszTunerName) const;
	const TVTest::String &GetCasReaderNameByTunerName(LPCTSTR pszTunerName) const;

private:
	struct CasLibraryNetworkInfo
	{
		WORD NetworkID;
		WORD TSID;
		TVTest::String FileName;
	};

	struct TunerCasInfo
	{
		TVTest::String TunerName;
		TVTest::String DeviceName;
		TVTest::String ReaderName;
	};

	TVTest::String m_DefaultCasLibrary;
	std::vector<CasLibraryNetworkInfo> m_CasLibraryNetworkMap;
	std::vector<TunerCasInfo> m_TunerCasMap;

	std::vector<TunerCasInfo>::const_iterator FindTunerCasMap(LPCTSTR pszTunerName) const;
};


#endif
