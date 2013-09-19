#ifndef CAPTURE_H
#define CAPTURE_H


#include "BasicWindow.h"
#include "StatusView.h"


class CCaptureImage
{
	HGLOBAL m_hData;
	bool m_fLocked;
	SYSTEMTIME m_stCaptureTime;
	CDynamicString m_Comment;

public:
	CCaptureImage(HGLOBAL hData);
	CCaptureImage(const BITMAPINFO *pbmi,const void *pBits);
	~CCaptureImage();
	bool SetClipboard(HWND hwnd);
	bool GetBitmapInfoHeader(BITMAPINFOHEADER *pbmih) const;
	bool LockData(BITMAPINFO **ppbmi,BYTE **ppBits);
	bool UnlockData();
	const SYSTEMTIME &GetCaptureTime() const { return m_stCaptureTime; }
	bool SetComment(LPCTSTR pszComment);
	LPCTSTR GetComment() const { return m_Comment.Get(); }
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
		virtual void OnRButtonDown(int x,int y) {}
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

class CCaptureWindow : public CCustomWindow
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
// CCustomWindow
	bool SetImage(const BITMAPINFO *pbmi,const void *pBits);
	bool SetImage(CCaptureImage *pImage);
	bool ClearImage();
	bool HasImage() const;
	bool SetEventHandler(CEventHandler *pEventHandler);
	void ShowStatusBar(bool fShow);
	bool IsStatusBarVisible() const { return m_fShowStatusBar; }
	void SetStatusTheme(const CStatusView::ThemeInfo *pTheme);

private:
	class CPreviewEventHandler : public CCapturePreview::CEventHandler
	{
		CCaptureWindow *m_pCaptureWindow;
	public:
		CPreviewEventHandler(CCaptureWindow *pCaptureWindow);
		void OnRButtonDown(int x,int y);
		bool OnKeyDown(UINT KeyCode,UINT Flags);
	};

	enum {
		STATUS_ITEM_CAPTURE,
		//STATUS_ITEM_CONTINUOUS,
		STATUS_ITEM_SAVE,
		STATUS_ITEM_COPY
	};

	class CCaptureStatusItem : public CStatusItem {
		DrawUtil::CMonoColorBitmap &m_IconBitmap;
	public:
		CCaptureStatusItem(DrawUtil::CMonoColorBitmap &IconBitmap);
		LPCTSTR GetName() const { return TEXT("キャプチャ"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
		void OnRButtonDown(int x,int y);
	};

	/*
	class CContinuousStatusItem : public CStatusItem {
		DrawUtil::CMonoColorBitmap &m_IconBitmap;
	public:
		CContinuousStatusItem(DrawUtil::CMonoColorBitmap &IconBitmap);
		LPCTSTR GetName() const { return TEXT("連写"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
		void OnRButtonDown(int x,int y);
	};
	*/

	class CSaveStatusItem : public CStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		DrawUtil::CMonoColorBitmap &m_IconBitmap;
	public:
		CSaveStatusItem(CCaptureWindow *pCaptureWindow,DrawUtil::CMonoColorBitmap &IconBitmap);
		LPCTSTR GetName() const { return TEXT("保存"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
	};

	class CCopyStatusItem : public CStatusItem {
		CCaptureWindow *m_pCaptureWindow;
		DrawUtil::CMonoColorBitmap &m_IconBitmap;
	public:
		CCopyStatusItem(CCaptureWindow *pCaptureWindow,DrawUtil::CMonoColorBitmap &IconBitmap);
		LPCTSTR GetName() const { return TEXT("コピー"); }
		void Draw(HDC hdc,const RECT *pRect);
		void OnLButtonDown(int x,int y);
	};

	static HINSTANCE m_hinst;

	CCapturePreview m_Preview;
	CPreviewEventHandler m_PreviewEventHandler;
	CStatusView m_Status;
	bool m_fShowStatusBar;
	DrawUtil::CMonoColorBitmap m_StatusIcons;
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
