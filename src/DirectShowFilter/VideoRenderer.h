#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H


#include "../BonTsEngine/Exception.h"


// ‰f‘œƒŒƒ“ƒ_ƒ‰Šî’êƒNƒ‰ƒX
class __declspec(novtable) CVideoRenderer : public CBonErrorHandler
{
protected:
	IBaseFilter *m_pRenderer;
	IGraphBuilder *m_pFilterGraph;
	HWND m_hwndRender;
	bool m_bCrop1088To1080;

public:
	enum RendererType {
		RENDERER_UNDEFINED=-1,
		RENDERER_DEFAULT,
		//RENDERER_VIDEORENDERER,
		RENDERER_VMR7,
		RENDERER_VMR9,
		RENDERER_VMR7RENDERLESS,
		RENDERER_VMR9RENDERLESS,
		RENDERER_EVR,
		RENDERER_OVERLAYMIXER,
		RENDERER_madVR
	};
	CVideoRenderer();
	virtual ~CVideoRenderer();
	virtual bool Initialize(IGraphBuilder *pFilterGraph,IPin *pInputPin,HWND hwndRender,HWND hwndMessageDrain)=0;
	virtual bool Finalize() { return true; }
	virtual bool SetVideoPosition(int SourceWidth,int SourceHeight,const RECT *pSourceRect,
							const RECT *pDestRect,const RECT *pWindowRect)=0;
	virtual bool GetDestPosition(RECT *pRect)=0;
	virtual bool GetCurrentImage(void **ppBuffer) { return false; }
	virtual bool ShowCursor(bool fShow) { return true; }
	virtual bool RepaintVideo(HWND hwnd,HDC hdc) { return true; }
	virtual bool DisplayModeChanged() { return true; }
	virtual bool SetVisible(bool fVisible) { return true; }
	virtual bool ShowProperty(HWND hwndOwner);
	virtual bool HasProperty();
	IBaseFilter *GetRendererFilter() const { return m_pRenderer; }
	virtual bool SetCrop1088To1080(bool bCrop) { return false; }

	static bool CreateRenderer(RendererType Type,CVideoRenderer **ppRenderer);
	static LPCTSTR EnumRendererName(int Index);
	static RendererType ParseName(LPCTSTR pszName);
	static bool IsAvailable(RendererType Type);
};


#endif
