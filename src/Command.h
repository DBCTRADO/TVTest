#ifndef TVTEST_COMMAND_H
#define TVTEST_COMMAND_H


#include <vector>
#include <unordered_map>


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
	int NumCommands() const;
	int GetCommandID(int Index) const;
	LPCTSTR GetCommandText(int Index) const;
	LPCTSTR GetCommandTextByID(int ID) const;
	int GetCommandName(int Index,LPTSTR pszName,int MaxLength) const;
	int GetCommandNameByID(int ID,LPTSTR pszName,int MaxLength) const;
	int IDToIndex(int ID) const;
	int ParseText(LPCTSTR pszText) const;
	bool RegisterCommand(int ID,LPCTSTR pszText,LPCTSTR pszName=nullptr);
	bool AddCommandCustomizer(CCommandCustomizer *pCustomizer);

private:
	void RegisterDefaultCommands();

	struct CommandInfo {
		int ID;
		TVTest::String Text;
		TVTest::String Name;
	};

	std::vector<CommandInfo> m_CommandList;
	std::unordered_map<TVTest::String,int,
		TVTest::StringFunctional::HashNoCase,
		TVTest::StringFunctional::EqualNoCase> m_CommandTextMap;
	std::unordered_map<int,size_t> m_CommandIDMap;
	std::vector<CCommandCustomizer*> m_CustomizerList;
};


#endif
