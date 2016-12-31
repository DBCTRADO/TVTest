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

	enum {
		COMMAND_STATE_DISABLED	=0x00000001U,
		COMMAND_STATE_CHECKED	=0x00000002U
	};

	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual ~CEventHandler() {}
		virtual void OnCommandStateChanged(int ID,unsigned int OldState,unsigned int NewState) {}
		virtual void OnCommandRadioCheckedStateChanged(int FirstID,int LastID,int CheckedID) {}
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
	int GetCommandShortName(int Index,LPTSTR pszName,int MaxLength) const;
	int GetCommandShortNameByID(int ID,LPTSTR pszName,int MaxLength) const;
	int IDToIndex(int ID) const;
	int ParseText(LPCTSTR pszText) const;
	bool RegisterCommand(int ID,LPCTSTR pszText,
						 LPCTSTR pszName=nullptr,LPCTSTR pszShortName=nullptr,
						 unsigned int State=0);
	bool AddCommandCustomizer(CCommandCustomizer *pCustomizer);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetCommandStateByID(int ID,unsigned int State);
	bool SetCommandStateByID(int ID,unsigned int Mask,unsigned int State);
	unsigned int GetCommandStateByID(int ID) const;
	bool SetCommandRadioCheckedState(int FirstID,int LastID,int CheckedID);

private:
	void RegisterDefaultCommands();

	struct CommandInfo {
		int ID;
		unsigned int State;
		TVTest::String Text;
		TVTest::String Name;
		TVTest::String ShortName;
	};

	std::vector<CommandInfo> m_CommandList;
	std::unordered_map<TVTest::String,int,
		TVTest::StringFunctional::HashNoCase,
		TVTest::StringFunctional::EqualNoCase> m_CommandTextMap;
	std::unordered_map<int,size_t> m_CommandIDMap;
	std::vector<CCommandCustomizer*> m_CustomizerList;
	CEventHandler *m_pEventHandler;
};


#endif
