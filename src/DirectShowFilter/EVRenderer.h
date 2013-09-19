#ifndef EVRENDERER_H
#define EVRENDERER_H


/*
	EVRのバグ(?)によりウィンドウの一部を転送先に指定すると
	バックバッファがクリアされずにフリッカが発生するので、
	転送用のウィンドウを作成する
*/
#define EVR_USE_VIDEO_WINDOW

#include "VideoRenderer.h"


class CVideoRenderer_EVR : public CVideoRenderer
{
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
};


#endif
