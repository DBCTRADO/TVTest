#include "stdafx.h"
#include "TVTest.h"
#include "IniFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace TVTest
{

	CIniFile::CIniFile()
		: m_OpenFlags(0)
		, m_pCurSection(nullptr)
		, m_hFile(INVALID_HANDLE_VALUE)
	{
	}

	CIniFile::~CIniFile()
	{
		Close();
	}

	bool CIniFile::Open(LPCWSTR pszFileName,UINT Flags)
	{
		Close();

		TRACE(TEXT("CIniFile::Open( \"%s\", 0x%x)\n"),pszFileName,Flags);

		if (IsStringEmpty(pszFileName)
				|| (Flags & (OPEN_READ | OPEN_WRITE))==0
				|| (Flags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==(OPEN_WRITE | OPEN_WRITE_VOLATILE))
			return false;

		String MutexName;
		MutexName=APP_NAME L"_Ini_Mutex_";
		MutexName+=pszFileName;
		StringUtility::Replace(MutexName,L'\\',L':');
		if (m_FileLock.Create(MutexName.c_str())) {
			if (!m_FileLock.Wait(5000)) {
				TRACE(TEXT("Ini file locked\n"));
				m_FileLock.Close();
				//return false;
			}
		}

		DWORD DesiredAccess=0,ShareMode=0;
		if ((Flags & OPEN_WRITE)!=0) {
			DesiredAccess|=GENERIC_READ | GENERIC_WRITE;
		} else {
			DesiredAccess|=GENERIC_READ;
			ShareMode|=FILE_SHARE_READ;
		}

		HANDLE hFile=::CreateFile(pszFileName,DesiredAccess,ShareMode,nullptr,
								  (Flags&OPEN_WRITE)!=0?OPEN_ALWAYS:OPEN_EXISTING,
								  FILE_ATTRIBUTE_NORMAL,nullptr);
		BYTE *pBuffer=nullptr;
		try {
			if (hFile!=INVALID_HANDLE_VALUE) {
				LARGE_INTEGER FileSize;
				if (!::GetFileSizeEx(hFile,&FileSize) || FileSize.QuadPart>MAX_FILE_SIZE) {
					throw __LINE__;
				}

				if (FileSize.LowPart>2) {
					pBuffer=new BYTE[FileSize.LowPart+3];
					DWORD Read;
					if (!::ReadFile(hFile,pBuffer,FileSize.LowPart,&Read,nullptr)
							|| Read!=FileSize.LowPart) {
						throw __LINE__;
					}
					pBuffer[FileSize.LowPart+0]=0;
					pBuffer[FileSize.LowPart+1]=0;
					pBuffer[FileSize.LowPart+2]=0;

					if (pBuffer[0]==0xFF && pBuffer[1]==0xFE) {
						Parse((WCHAR*)(pBuffer+2));
					} else {
						int Length=::MultiByteToWideChar(CP_ACP,0,(char*)pBuffer,FileSize.LowPart,nullptr,0);
						LPWSTR pszData=new WCHAR[Length+1];
						::MultiByteToWideChar(CP_ACP,0,(char*)pBuffer,FileSize.LowPart,pszData,Length);
						pszData[Length]=L'\0';
						Parse(pszData);
						delete [] pszData;
					}

					delete [] pBuffer;
				}
			} else {
				TRACE(TEXT("Ini file open failed 0x%x\n"),::GetLastError());
				throw __LINE__;
			}
		} catch (...) {
			delete [] pBuffer;
			if (hFile!=INVALID_HANDLE_VALUE)
				::CloseHandle(hFile);
			m_FileLock.Release();
			m_FileLock.Close();
			return false;
		}

		if ((Flags & OPEN_WRITE)==0) {
			::CloseHandle(hFile);
			m_FileLock.Release();
			m_FileLock.Close();
		} else {
			m_hFile=hFile;
		}

		m_FileName=pszFileName;
		m_OpenFlags=Flags;

		return true;
	}

	bool CIniFile::Close()
	{
		if (m_OpenFlags==0)
			return true;

		bool fOK=true;

		if ((m_OpenFlags&OPEN_WRITE)!=0 && m_hFile!=INVALID_HANDLE_VALUE) {
			String Buffer;

			for (auto itrSection=m_SectionList.begin();itrSection!=m_SectionList.end();++itrSection) {
				if (!itrSection->SectionName.empty()) {
					Buffer+=L"[";
					Buffer+=itrSection->SectionName;
					Buffer+=L"]\r\n";
				}

				for (auto itrValue=itrSection->Entries.begin();itrValue!=itrSection->Entries.end();++itrValue) {
					if (!itrValue->Name.empty()) {
						Buffer+=itrValue->Name;
						Buffer+=L"=";
					}
					Buffer+=itrValue->Value;
					Buffer+=L"\r\n";
				}
			}

			::SetFilePointer(m_hFile,0,nullptr,FILE_BEGIN);

			const DWORD Size=(DWORD)(Buffer.size()*sizeof(String::value_type));
			DWORD Write;
			static const WORD BOM=0xFEFF;
			if (!::WriteFile(m_hFile,&BOM,sizeof(BOM),&Write,nullptr) || Write!=sizeof(BOM)
					|| !::WriteFile(m_hFile,Buffer.data(),Size,&Write,nullptr)
					|| Write!=Size) {
				fOK=false;
			} else {
				::SetEndOfFile(m_hFile);
				::FlushFileBuffers(m_hFile);
			}
		}

		if (m_hFile!=INVALID_HANDLE_VALUE) {
			::CloseHandle(m_hFile);
			m_hFile=INVALID_HANDLE_VALUE;
		}

		m_FileLock.Release();
		m_FileLock.Close();

		m_FileName.clear();
		m_Section.clear();
		m_OpenFlags=0;

		m_SectionList.clear();
		m_pCurSection=nullptr;

		return fOK;
	}

	bool CIniFile::SelectSection(LPCWSTR pszSection)
	{
		if (m_OpenFlags==0)
			return false;

		m_Section.clear();
		m_pCurSection=nullptr;

		if (IsStringEmpty(pszSection))
			return false;

		auto i=FindSection(pszSection);
		if (i==m_SectionList.end()) {
			if ((m_OpenFlags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==0)
				return false;

			CreateSection(pszSection);
			m_pCurSection=&m_SectionList.back();
		} else {
			m_pCurSection=&*i;
		}

		m_Section=pszSection;

		return true;
	}

	bool CIniFile::IsSectionExists(LPCWSTR pszSection)
	{
		if (IsStringEmpty(pszSection))
			return false;

		return FindSection(pszSection)!=m_SectionList.end();
	}

	bool CIniFile::DeleteSection(LPCWSTR pszSection)
	{
		if ((m_OpenFlags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==0
				|| IsStringEmpty(pszSection))
			return false;

		auto i=FindSection(pszSection);
		if (i==m_SectionList.end())
			return false;

		m_SectionList.erase(i);
		m_pCurSection=nullptr;

		return true;
	}

	bool CIniFile::ClearSection(LPCWSTR pszSection)
	{
		if ((m_OpenFlags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==0
				|| IsStringEmpty(pszSection))
			return false;

		auto i=FindSection(pszSection);
		if (i==m_SectionList.end())
			return false;

		i->Entries.clear();

		return true;
	}

	bool CIniFile::ClearSection()
	{
		if (m_Section.empty())
			return false;

		return ClearSection(m_Section.c_str());
	}

	bool CIniFile::GetValue(LPCWSTR pszName,String *pValue)
	{
		if (pValue==nullptr)
			return false;

		pValue->clear();

		if ((m_OpenFlags & OPEN_READ)==0
				|| m_Section.empty()
				|| IsStringEmpty(pszName))
			return false;

		CSectionData *pSection=GetCurSection();
		if (pSection==nullptr)
			return false;

		auto itrValue=FindValue(pSection->Entries,pszName);
		if (itrValue==pSection->Entries.end())
			return false;

		*pValue=itrValue->Value;
		if (pValue->length()>=2
				&& pValue->at(0)==L'"'
				&& pValue->at(pValue->length()-1)==L'"') {
			if (pValue->length()>2)
				*pValue=pValue->substr(1,pValue->length()-2);
			else
				pValue->clear();
		}

		return true;
	}

	bool CIniFile::SetValue(LPCWSTR pszName,LPCWSTR pszValue)
	{
		if ((m_OpenFlags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==0
				|| m_Section.empty()
				|| IsStringEmpty(pszName))
			return false;

		CSectionData *pSection=GetCurSection();
		if (pSection==nullptr)
			return false;

		auto itrValue=FindValue(pSection->Entries,pszName);
		if (itrValue==pSection->Entries.end()) {
			//TRACE(TEXT("新規設定 : %s=%s\n"),pszName,pszValue);
			bool fInserted=false;
			for (auto i=pSection->Entries.rbegin();i!=pSection->Entries.rend();i++) {
				if (!i->Name.empty()) {
					pSection->Entries.insert(i.base(),CEntry(pszName,pszValue));
					fInserted=true;
					break;
				}
			}
			if (!fInserted)
				pSection->Entries.push_front(CEntry(pszName,pszValue));
		} else {
			if (IsStringEmpty(pszValue))
				itrValue->Value.clear();
			else
				itrValue->Value=pszValue;
		}

		return true;
	}

	bool CIniFile::IsValueExists(LPCWSTR pszName)
	{
		if (m_Section.empty()
				|| IsStringEmpty(pszName))
			return false;

		CSectionData *pSection=GetCurSection();
		if (pSection==nullptr)
			return false;

		auto itrValue=FindValue(pSection->Entries,pszName);
		return itrValue!=pSection->Entries.end();
	}

	bool CIniFile::DeleteValue(LPCWSTR pszName)
	{
		if ((m_OpenFlags & (OPEN_WRITE | OPEN_WRITE_VOLATILE))==0
				|| m_Section.empty()
				|| IsStringEmpty(pszName))
			return false;

		CSectionData *pSection=GetCurSection();
		if (pSection==nullptr)
			return false;

		auto itrValue=FindValue(pSection->Entries,pszName);
		if (itrValue==pSection->Entries.end())
			return false;

		pSection->Entries.erase(itrValue);

		return true;
	}

	bool CIniFile::GetSectionEntries(LPCWSTR pszSection,std::vector<CEntry> *pEntries)
	{
		if (pEntries==nullptr)
			return false;

		pEntries->clear();

		if ((m_OpenFlags & OPEN_READ)==0
				|| IsStringEmpty(pszSection))
			return false;

		auto itrSection=FindSection(pszSection);
		if (itrSection==m_SectionList.end())
			return false;

		for (auto i=itrSection->Entries.begin();i!=itrSection->Entries.end();i++) {
			if (!i->Name.empty()) {
				pEntries->push_back(*i);
			}
		}

		return true;
	}

	bool CIniFile::GetEntries(std::vector<CEntry> *pEntries)
	{
		if (m_Section.empty())
			return false;

		return GetSectionEntries(m_Section.c_str(),pEntries);
	}

	bool CIniFile::Parse(LPCWSTR pszBuffer)
	{
		int LineCount=0;
		String Line;

		LPCWSTR p=pszBuffer;
		while (*p!=L'\0') {
			String::size_type Length=0;
			for (;p[Length]!=L'\0' && p[Length]!=L'\r' && p[Length]!=L'\n';Length++);
			Line.assign(p,Length);

			String::size_type End;
			if (Length>3 && Line[0]==L'[' && (End=Line.find(L']'))!=String::npos) {
				String Name(Line.substr(1,End-1));
				StringUtility::Trim(Name);
				CreateSection(Name.c_str());
			} else {
				//TRACE(L"%4d : %s\n",LineCount+1,Line.c_str());
				if (m_SectionList.empty())
					CreateSection(L"");
				m_SectionList.back().Entries.push_back(CEntry(Line));
			}

			LineCount++;
			p+=Length;
			if (*p==L'\r')
				p++;
			if (*p==L'\n')
				p++;
		}

		return true;
	}

	CIniFile::SectionList::iterator CIniFile::FindSection(LPCWSTR pszName)
	{
		if (IsStringEmpty(pszName))
			return m_SectionList.end();

		for (auto i=m_SectionList.begin();i!=m_SectionList.end();i++) {
			if (StringUtility::CompareNoCase(i->SectionName,pszName)==0)
				return i;
		}

		return m_SectionList.end();
	}

	CIniFile::SectionList::const_iterator CIniFile::FindSection(LPCWSTR pszName) const
	{
		if (IsStringEmpty(pszName))
			return m_SectionList.end();

		for (auto i=m_SectionList.begin();i!=m_SectionList.end();i++) {
			if (StringUtility::CompareNoCase(i->SectionName,pszName)==0)
				return i;
		}

		return m_SectionList.end();
	}

	CIniFile::EntryList::iterator CIniFile::FindValue(EntryList &Entries,LPCWSTR pszName)
	{
		for (auto i=Entries.begin();i!=Entries.end();i++) {
			if (!i->Name.empty() && StringUtility::CompareNoCase(i->Name,pszName)==0)
				return i;
		}

		return Entries.end();
	}

	CIniFile::EntryList::const_iterator CIniFile::FindValue(const EntryList &Entries,LPCWSTR pszName) const
	{
		for (auto i=Entries.begin();i!=Entries.end();i++) {
			if (!i->Name.empty() && StringUtility::CompareNoCase(i->Name,pszName)==0)
				return i;
		}

		return Entries.end();
	}

	bool CIniFile::CreateSection(LPCWSTR pszName)
	{
		//TRACE(L"INIセクション作成 : [%s]\n",pszName);

		m_SectionList.push_back(CSectionData(pszName));

		if (!IsStringEmpty(pszName)
				&& m_SectionList.size()>1) {
			auto i=m_SectionList.rbegin();
			i++;
			if (!i->SectionName.empty()
					&& (i->Entries.empty()
						|| !i->Entries.back().Name.empty()
						|| !i->Entries.back().Value.empty())) {
				i->Entries.push_back(CEntry());
			}
		}

		return true;
	}

	CIniFile::CSectionData *CIniFile::GetCurSection()
	{
		if (m_Section.empty())
			return nullptr;

		if (m_pCurSection!=nullptr) {
			auto itrSection=FindSection(m_Section.c_str());
			if (itrSection==m_SectionList.end())
				return nullptr;
			m_pCurSection=&*itrSection;
		}

		return m_pCurSection;
	}


	CIniFile::CEntry::CEntry(const String &Text)
	{
		String::size_type Pos;

		Pos=Text.find_first_not_of(L" \t");
		if (Pos!=String::npos && Text[Pos]==L';') {
			Value=Text;
		} else {
			Pos=Text.find(L'=');
			if (Pos!=String::npos) {
				Name=Text.substr(0,Pos);
				StringUtility::Trim(Name);
				Value=Text.substr(Pos+1);
			} else {
				Value=Text;
			}
		}
	}

	CIniFile::CEntry::CEntry(LPCWSTR pszName,LPCWSTR pszValue)
	{
		if (!IsStringEmpty(pszName))
			Name=pszName;
		if (!IsStringEmpty(pszValue))
			Value=pszValue;
	}

}
