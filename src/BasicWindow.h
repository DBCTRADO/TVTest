#ifndef TVTEST_BASIC_WINDOW_H
#define TVTEST_BASIC_WINDOW_H


#undef GetWindowStyle
#undef GetWindowExStyle
inline DWORD GetWindowStyle(HWND hwnd) {
	return ::GetWindowLong(hwnd, GWL_STYLE);
}
inline DWORD GetWindowExStyle(HWND hwnd) {
	return ::GetWindowLong(hwnd, GWL_EXSTYLE);
}


namespace TVTest
{

	// ウィンドウの基底クラス
	class ABSTRACT_CLASS(CBasicWindow)
	{
	protected:
		HWND m_hwnd;
		struct {
			int Left, Top;
			int Width, Height;
			bool fMaximized;
		} m_WindowPosition;

		bool CreateBasicWindow(
			HWND hwndParent, DWORD Style, DWORD ExStyle, int ID,
			LPCTSTR pszClassName, LPCTSTR pszText, HINSTANCE hinst);
		static CBasicWindow *OnCreate(HWND hwnd, LPARAM lParam);
		void OnDestroy();
		static CBasicWindow *GetBasicWindow(HWND hwnd);

	public:
		CBasicWindow();
		virtual ~CBasicWindow();

		virtual bool Create(HWND hwndParent, DWORD Style, DWORD ExStyle = 0, int ID = 0) = 0;
		bool IsCreated() const { return m_hwnd != nullptr; }
		void Destroy();
		bool SetPosition(int Left, int Top, int Width, int Height);
		bool SetPosition(const RECT * pPosition);
		void GetPosition(int *pLeft, int *pTop, int *pWidth, int *pHeight) const;
		void GetPosition(RECT * pPosition) const;
		int GetWidth() const;
		int GetHeight() const;
		bool GetScreenPosition(RECT * pPosition) const;
		virtual void SetVisible(bool fVisible);
		bool GetVisible() const;
		bool SetMaximize(bool fMaximize);
		bool GetMaximize() const;
		HWND GetHandle() const { return m_hwnd; }
		bool Invalidate(bool fErase = true);
		bool Invalidate(const RECT * pRect, bool fErase = true);
		bool Update();
		bool Redraw(const RECT *pRect = nullptr, UINT Flags = RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
		bool GetClientRect(RECT * pRect) const;
		bool GetClientSize(SIZE * pSize) const;
		bool SetParent(HWND hwnd);
		bool SetParent(CBasicWindow * pWindow);
		HWND GetParent() const;
		bool MoveToMonitorInside();
		DWORD GetWindowStyle() const;
		bool SetWindowStyle(DWORD Style, bool fFrameChange = false);
		DWORD GetWindowExStyle() const;
		bool SetWindowExStyle(DWORD ExStyle, bool fFrameChange = false);
		LRESULT SendMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
		bool PostMessage(UINT Msg, WPARAM wParam, LPARAM lParam);
		bool SendSizeMessage();
		bool SetOpacity(int Opacity, bool fClearLayered = true);
	};

	class ABSTRACT_CLASS(CCustomWindow)
		: public CBasicWindow
	{
	protected:
		static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		virtual LRESULT OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	public:
		CCustomWindow();
		virtual ~CCustomWindow();
	};

}	// namespace TVTest


#endif
