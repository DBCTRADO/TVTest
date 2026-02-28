/*
  TVTest
  Copyright(c) 2008-2020 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#ifndef TVTEST_WHEEL_COMMAND_H
#define TVTEST_WHEEL_COMMAND_H


#include <vector>


namespace TVTest
{

	class CWheelCommandManager
	{
	public:
		static constexpr int MAX_COMMAND_PARSABLE_NAME = 32;
		static constexpr int MAX_COMMAND_TEXT = 64;

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
