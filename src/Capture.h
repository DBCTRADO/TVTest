#ifndef CAPTURE_H
#define CAPTURE_H


#include "BasicWindow.h"
#include "StatusView.h"


class CCaptureImage
{
	HGLOBAL m_hData;
	bool m_fLocked;
	SYSTEMTIME m_stCaptureTime;
	TVTest::String m_Comment;

public:
	CCaptureImage(HGLOBAL hData);
	CCaptureImage(const BITMAPINFO *pbmi,const void *pBits);
	~CCaptureImage();
	bool SetClipboard(HWND hwnd);
	bool GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const;
	bool LockData(BITMAPINFO **ppbmi,BYTE **ppBits);
	bool UnlockData();
	const SYSTEMTIME &GetCaptureTime() const { return m_stCaptureTime; }
	void SetComment(LPCTSTR pszComment);
	LPCTSTR GetComment() const;
};

class CCapturePreview : public CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CCapturePreview *m_pCapturePreview;
	public:
		CEventHandler();
		virtual ~CEventHandler()=0;
		virtual void OnLButtonDown(int x,int y) {}
		virtual void OnRButtonUp(int x,int y) {}
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		friend class CCapturePreview;
	};

	static bool Initialize(HINSTANCE hinst);

	CCapturePreview();
	~CCapturePreview();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CCapturePreview
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CEventHandler *pEventHandler);

private:
	static HINSTANCE m_hinst;

	CCaptureImage *m_pImage;
	COLORREF m_crBackColor;
	CEventHandler *m_pEventHandler;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

class CCaptureWindow
	: public CCustomWindow
	, public TVTest::CUIBase
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CCaptureWindow *m_pCaptureWindow;
	public:
		CEventHandler();
		virtual ~CEventHandler()=0;
		virtual void OnRestoreSettings() {}
		virtual bool OnClose() { return true; }
		virtual bool OnSave(CCaptureImage *pImage) { return false; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual bool OnActivate(bool fActive) { return false; }
		friend class CCaptureWindow;
	};

	static bool Initialize(HINSTANCE hinst);

	CCaptureWindow();
	~CCaptureWindow();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CUIBase
	void SetTheme(const TVTest::Theme::CThemeManager *pThemeManager) override;

// CCaptureWindow
	bool SetImage(const BITMAPINFO *pbmi,const void *pBits);
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	void ShowStatusBar(bool fShow);
	bool IsStatusBarVisible() const { return m_fShowStatusBar; }

private:
	class CPreviewEventHandler : public CCapturePreview::CEventHandler
	{
		CCaptureWindow *m_pCaptureWindow;
	public:
		CPreviewEventHandler(CCaptureWindow *pCaptureWindow);
		void OnRButtonUp(int x,int y);
		bool OnKeyDown(UINT KeyCode,UINT Flags);
	};

	enum {
		STATUS_ITEM_CAPTURE,
		//STATUS_ITEM_CONTINUOUS,
		STATUS_ITEM_SAVE,
		STATUS_ITEM_COPY
	};

	class CCaptureStatusItem : public CIconStatusItem {
		DrawUtil::CMonoColorIconList &m_Icons;
	public:
		CCaptureStatusItem(DrawUtil::CMonoColorIconList &Icons);
		LPCTSTR GetIDText() const override { return TEXT("Capture"); }
		LPCTSTR GetName() const override { return TEXT("キャプチャ"); }
		void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
		void OnLButtonDown(int x,int y) override;
		void OnRButtonDown(int x,int y) override;
	};

	class CSaveStatusItem : public CIconStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		DrawUtil::CMonoColorIconList &m_Icons;
	public:
		CSaveStatusItem(CCaptureWindow *pCaptureWindow,DrawUtil::CMonoColorIconList &Icons);
		LPCTSTR GetIDText() const override { return TEXT("Save"); }
		LPCTSTR GetName() const override { return TEXT("保存"); }
		void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
		void OnLButtonDown(int x,int y) override;
	};

	class CCopyStatusItem : public CIconStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		DrawUtil::CMonoColorIconList &m_Icons;
	public:
		CCopyStatusItem(CCaptureWindow *pCaptureWindow,DrawUtil::CMonoColorIconList &Icons);
		LPCTSTR GetIDText() const override { return TEXT("Copy"); }
		LPCTSTR GetName() const override { return TEXT("コピー"); }
		void Draw(HDC hdc,const RECT &ItemRect,const RECT &DrawRect,unsigned int Flags) override;
		void OnLButtonDown(int x,int y) override;
	};

	static HINSTANCE m_hinst;

	CCapturePreview m_Preview;
	CPreviewEventHandler m_PreviewEventHandler;
	CStatusView m_Status;
	bool m_fShowStatusBar;
	DrawUtil::CMonoColorIconList m_StatusIcons;
	CCaptureImage *m_pImage;
	CEventHandler *m_pEventHandler;
	bool m_fCreateFirst;

	void SetTitle();
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

/*
class CImageSaveThread {
	CCaptureImage *m_pImage;
	LPTSTR m_pszFileName;
	int m_Format;
	LPTSTR m_pszOption;
	LPTSTR m_pszComment;
	CImageCodec m_ImageCodec;
	static void SaveProc(void *pParam);
public:
	CImageSaveThread(CCaptureImage *pImage,LPCTSTR pszFileName,int Format,
										LPCTSTR pszOption,LPCTSTR pszComment);
	~CImageSaveThread();
	bool BeginSave();
};
*/


#endif
