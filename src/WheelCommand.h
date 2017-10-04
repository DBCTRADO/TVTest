#ifndef TVTEST_WHEEL_COMMAND_H
#define TVTEST_WHEEL_COMMAND_H


#include <vector>


namespace TVTest
{

	class CWheelCommandManager
	{
	public:
		static const int MAX_COMMAND_PARSABLE_NAME = 32;
		static const int MAX_COMMAND_TEXT = 64;

		CWheelCommandManager();
		int GetCommandCount() const;
		int GetCommandID(int Index) const;
		int GetCommandParsableName(int ID, LPTSTR pszName, int MaxName) const;
		int GetCommandText(int ID, LPTSTR pszText, int MaxText) const;
		int ParseCommand(LPCTSTR pszCommand) const;

	private:
		struct CommandInfo
		{
			int ID;
			String IDText;
		};

		std::vector<CommandInfo> m_CommandList;
	};

}


#endif
