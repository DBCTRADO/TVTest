#ifndef ZOOM_OPTIONS_H
#define ZOOM_OPTIONS_H


#include "Command.h"
#include "Settings.h"
#include "Dialog.h"


class CZoomOptions
	: public CBasicDialog
	, public CSettingsBase
	, public CCommandList::CCommandCustomizer
{
public:
	enum ZoomType {
		ZOOM_RATE,
		ZOOM_SIZE
	};

	struct ZoomRate {
		int Rate;
		int Factor;

		int GetPercentage() const { return Factor!=0?Rate*100/Factor:0; }
	};

	struct ZoomSize {
		int Width;
		int Height;
	};

	struct ZoomInfo {
		ZoomType Type;
		ZoomRate Rate;
		ZoomSize Size;
		bool fVisible;
	};

	enum { NUM_ZOOM_COMMANDS = 11+10 };
	enum { MAX_RATE = 1000 };

	CZoomOptions();
	~CZoomOptions();

//CBasicDialog
	bool Show(HWND hwndOwner) override;

//CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

//CZoomOptions
	bool SetMenu(HMENU hmenu,const ZoomInfo *pCurZoom) const;
	bool GetZoomInfoByCommand(int Command,ZoomInfo *pInfo) const;

private:
	struct ZoomCommandInfo {
		int Command;
		ZoomInfo Info;
	};
	static const ZoomCommandInfo m_DefaultZoomList[NUM_ZOOM_COMMANDS];

	ZoomInfo m_ZoomList[NUM_ZOOM_COMMANDS];

	int m_Order[NUM_ZOOM_COMMANDS];
	bool m_fChanging;
	ZoomInfo m_ZoomSettingList[NUM_ZOOM_COMMANDS];

	int GetIndexByCommand(int Command) const;
	void FormatCommandText(int Command,const ZoomInfo &Info,LPTSTR pszText,int MaxLength) const;

	void SetItemState(HWND hDlg);
	int GetItemIndex(HWND hwndList,int Item);
	void UpdateItemText(HWND hDlg,int Item);

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

//CCommandCustomizer
	bool GetCommandName(int Command,LPTSTR pszName,int MaxLength) override;
};


#endif
