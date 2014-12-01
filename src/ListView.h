#ifndef TVTEST_LIST_VIEW_H
#define TVTEST_LIST_VIEW_H


namespace TVTest
{

	class CListView
	{
	public:
		CListView();
		~CListView();
		bool Attach(HWND hwnd);
		void Detach();
		void SetExtendedStyle(DWORD Style);
		bool InitCheckList();
		int InsertItem(int Index,LPCTSTR pszText,LPARAM Param=0);
		bool DeleteItem(int Index);
		void DeleteAllItems();
		bool SetItemText(int Index,LPCTSTR pszText);
		bool SetItemText(int Index,int SubItem,LPCTSTR pszText);
		bool SetItemState(int Index,UINT State,UINT Mask);
		bool CheckItem(int Index,bool fCheck);
		bool IsItemChecked(int Index) const;
		LPARAM GetItemParam(int Index) const;
		int GetItemCount() const;
		int GetSelectedItem() const;
		bool MoveItem(int From,int To);
		bool EnsureItemVisible(int Index,bool fPartialOK=false);
		int InsertColumn(int Index,LPCTSTR pszText,int Format=LVCFMT_LEFT);
		void AdjustColumnWidth(bool fUseHeader=true);

	protected:
		HWND m_hwnd;
	};

}	// namespace TVTest


#endif
