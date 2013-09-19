#ifndef TASKBAR_H
#define TASKBAR_H


class CTaskbarManager {
private:
	HWND m_hwnd;
	UINT m_TaskbarButtonCreatedMessage;
	interface ITaskbarList3 *m_pTaskbarList;

public:
	CTaskbarManager();
	~CTaskbarManager();
	bool Initialize(HWND hwnd);
	void Finalize();
	bool HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool SetRecordingStatus(bool fRecording);
	bool SetProgress(int Pos,int Max);
	bool EndProgress();
};


#endif
