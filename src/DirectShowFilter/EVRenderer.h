#ifndef EVRENDERER_H
#define EVRENDERER_H


/*
	EVR�̃o�O(?)�ɂ��E�B���h�E�̈ꕔ��]����Ɏw�肷���
	�o�b�N�o�b�t�@���N���A���ꂸ�Ƀt���b�J����������̂ŁA
	�]���p�̃E�B���h�E���쐬����
*/
#define EVR_USE_VIDEO_WINDOW

#include "VideoRenderer.h"


class CVideoRenderer_EVR : public CVideoRenderer
{
	HRESULT UpdateRenderingPrefs(interface IMFVideoDisplayControl *pDisplayControl);

	//HMODULE m_hMFPlatLib;
#ifdef EVR_USE_VIDEO_WINDOW
	HWND m_hwndVideo;
	HWND m_hwndMessageDrain;
	bool m_fShowCursor;
	static CVideoRenderer_EVR *GetThis(HWND hwnd);
	static LRESULT CALLBACK VideoWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
#endif

public:
	CVideoRenderer_EVR();
	~CVideoRenderer_EVR();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect);
	bool GetDestPosition(RECT *pRect);
	bool GetCurrentImage(void **ppBuffer);
	bool ShowCursor(bool fShow);
	bool RepaintVideo(HWND hwnd,HDC hdc);
	bool DisplayModeChanged();
	bool SetVisible(bool fVisible);
	bool SetClipToDevice(bool bClip);

protected:
	virtual HRESULT InitializePresenter(IBaseFilter *pFilter) { return S_OK; }
};


#endif
