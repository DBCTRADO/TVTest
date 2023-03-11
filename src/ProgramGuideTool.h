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


#ifndef TVTEST_PROGRAM_GUIDE_TOOL_H
#define TVTEST_PROGRAM_GUIDE_TOOL_H


#include "Dialog.h"
#include "GUIUtil.h"


namespace TVTest
{

	namespace ProgramGuide
	{
		class CServiceInfo;
	}


	class CProgramGuideTool
	{
	public:
		CProgramGuideTool() = default;
		CProgramGuideTool(const String &Name, const String &Command);
		CProgramGuideTool(LPCTSTR pszName, LPCTSTR pszCommand);

		LPCTSTR GetName() const { return m_Name.c_str(); }
		LPCTSTR GetCommand() const { return m_Command.c_str(); }
		bool GetPath(LPTSTR pszPath, int MaxLength) const;
		HICON GetIcon();
		bool Execute(
			const ProgramGuide::CServiceInfo *pServiceInfo,
			const LibISDB::EventInfo *pEventInfo, HWND hwnd);
		bool ShowDialog(HWND hwndOwner);

	private:
		class CProgramGuideToolDialog
			: public CBasicDialog
		{
		public:
			CProgramGuideToolDialog(CProgramGuideTool *pTool);
			bool Show(HWND hwndOwner) override;

		private:
			CProgramGuideTool *m_pTool;

			INT_PTR DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
		};

		String m_Name;
		String m_Command;
		CIcon m_Icon;

		static bool GetCommandFileName(LPCTSTR *ppszCommand, LPTSTR pszFileName, int MaxFileName);
	};

	class CProgramGuideToolList
	{
	public:
		CProgramGuideToolList() = default;
		CProgramGuideToolList(const CProgramGuideToolList &Src);
		CProgramGuideToolList &operator=(const CProgramGuideToolList &Src);

		void Clear();
		bool Add(CProgramGuideTool *pTool);
		CProgramGuideTool *GetTool(size_t Index);
		const CProgramGuideTool *GetTool(size_t Index) const;
		size_t NumTools() const { return m_ToolList.size(); }

	private:
		std::vector<std::unique_ptr<CProgramGuideTool>> m_ToolList;
	};

}	// namespace TVTest


#endif
