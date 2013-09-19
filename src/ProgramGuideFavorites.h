#ifndef PROGRAM_GUIDE_FAVORITES_H
#define PROGRAM_GUIDE_FAVORITES_H


#include <vector>
#include "Dialog.h"


class CProgramGuideFavorites
{
public:
	struct FavoriteInfo
	{
		TVTest::String Name;
		int Group;
		TVTest::String Label;
		COLORREF BackColor;
		COLORREF TextColor;

		void SetDefaultColors();
	};

	CProgramGuideFavorites();
	void Clear();
	size_t GetCount() const;
	bool Add(const FavoriteInfo &Info);
	bool Get(size_t Index,FavoriteInfo *pInfo) const;
	FavoriteInfo *Get(size_t Index);
	const FavoriteInfo *Get(size_t Index) const;
	bool Set(size_t Index,const FavoriteInfo &Info);
	bool GetFixedWidth() const { return m_fFixedWidth; }
	void SetFixedWidth(bool fFixed) { m_fFixedWidth=fFixed; }

private:
	std::vector<FavoriteInfo> m_List;
	bool m_fFixedWidth;
};

class CProgramGuideFavoritesDialog : public CBasicDialog
{
public:
	CProgramGuideFavoritesDialog(const CProgramGuideFavorites &Favorites);
	~CProgramGuideFavoritesDialog();

// CBasicDialog
	bool Show(HWND hwndOwner) override;

// CProgramGuideFavoritesDialog
	void AddNewItem(const CProgramGuideFavorites::FavoriteInfo &Info);
	const CProgramGuideFavorites &GetFavorites() const { return m_Favorites; }

private:
	CProgramGuideFavorites m_Favorites;
	int m_CurItem;
	bool m_fChanging;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetItemState(HWND hDlg);
	CProgramGuideFavorites::FavoriteInfo *GetCurItemInfo();
};


#endif
