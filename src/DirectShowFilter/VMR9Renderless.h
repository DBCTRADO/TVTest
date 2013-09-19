#ifndef VMR9_RENDERLESS_H
#define VMR9_RENDERLESS_H


#include <VideoRenderer.h>


class CVideoRenderer_VMR9Renderless : public CVideoRenderer {
	class CVMR9Allocator *m_pAllocator;
public:
	CVideoRenderer_VMR9Renderless();
	~CVideoRenderer_VMR9Renderless();
	bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain);
	bool Finalize();
	virtual bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
								const RECT *pDestRect,const RECT *pWindowRect);
	bool GetDestPosition(RECT *pRect);
	bool GetCurrentImage(void **ppBuffer);
	bool RepaintVideo(HWND hwnd,HDC hdc);
	bool DisplayModeChanged();
	bool SetVisible(bool fVisible);
};


#endif
