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
		bool InitCheckList();
		int InsertItem(int Index,LPCTSTR pszText,LPARAM Param=0);
		bool DeleteItem(int Index);
		void DeleteAllItems();
		bool SetItemText(int Index,LPCTSTR pszText);
		bool SetItemState(int Index,UINT State,UINT Mask);
		bool CheckItem(int Index,bool fCheck);
		bool IsItemChecked(int Index) const;
		LPARAM GetItemParam(int Index) const;
		int GetItemCount() const;
		int GetSelectedItem() const;
		bool MoveItem(int From,int To);
		bool EnsureItemVisible(int Index,bool fPartialOK=false);

	protected:
		HWND m_hwnd;
	};

}	// namespace TVTest


#endif
