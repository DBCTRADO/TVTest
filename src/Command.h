#ifndef TVTEST_COMMAND_H
#define TVTEST_COMMAND_H


#include <vector>


class CDriverManager;
class CPluginManager;

class CCommandList
{
public:
	enum {
		MAX_COMMAND_TEXT=MAX_PATH,
		MAX_COMMAND_NAME=MAX_PATH+16
	};

	class ABSTRACT_CLASS(CCommandCustomizer)
	{
	protected:
		int m_FirstID;
		int m_LastID;

	public:
		CCommandCustomizer(int FirstID,int LastID)
			: m_FirstID(FirstID), m_LastID(LastID) {}
		virtual ~CCommandCustomizer() {}
		virtual bool IsCommandValid(int Command) { return Command>=m_FirstID && Command<=m_LastID; }
		virtual bool GetCommandName(int Command,LPTSTR pszName,int MaxLength) { return false; }
	};

	CCommandList();
	~CCommandList();
	bool Initialize(const CDriverManager *pDriverManager,
					const CPluginManager *pPluginManager);
	int NumCommands() const;
	int GetCommandID(int Index) const;
	LPCTSTR GetCommandText(int Index) const;
	LPCTSTR GetCommandTextByID(int ID) const;
	int GetCommandName(int Index,LPTSTR pszName,int MaxLength) const;
	int GetCommandNameByID(int ID,LPTSTR pszName,int MaxLength) const;
	int IDToIndex(int ID) const;
	int ParseText(LPCTSTR pszText) const;
	bool AddCommandCustomizer(CCommandCustomizer *pCustomizer);

private:
	struct PluginCommandInfo {
		CDynamicString Text;
		CDynamicString Name;
		PluginCommandInfo(LPCTSTR pszText,LPCTSTR pszName) : Text(pszText), Name(pszName) {}
	};

	std::vector<CCommandCustomizer*> m_CustomizerList;
	std::vector<CDynamicString> m_DriverList;
	std::vector<CDynamicString> m_PluginList;
	std::vector<PluginCommandInfo> m_PluginCommandList;
};


#endif
