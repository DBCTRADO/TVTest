#include "stdafx.h"
#include "TVTest.h"
#include "RawInput.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


enum {
	RAWINPUT_DETAILS		= 0x0209,
	RAWINPUT_GUIDE			= 0x008D,
	RAWINPUT_TVJUMP			= 0x0025,
	RAWINPUT_STANDBY		= 0x0082,
	RAWINPUT_OEM1			= 0x0080,
	RAWINPUT_OEM2			= 0x0081,
	RAWINPUT_MYTV			= 0x0046,
	RAWINPUT_MYVIDEOS		= 0x004A,
	RAWINPUT_MYPICTURES		= 0x0049,
	RAWINPUT_MYMUSIC		= 0x0047,
	RAWINPUT_RECORDEDTV		= 0x0048,
	RAWINPUT_DVDANGLE		= 0x004B,
	RAWINPUT_DVDAUDIO		= 0x004C,
	RAWINPUT_DVDMENU		= 0x0024,
	RAWINPUT_DVDSUBTITLE	= 0x004D
};

static const struct {
	int RawData;
	LPCTSTR pszText;
} KeyList[] = {
	{RAWINPUT_DETAILS,		TEXT("Details")},
	{RAWINPUT_GUIDE,		TEXT("Guide")},
	{RAWINPUT_TVJUMP,		TEXT("TV Jump")},
	{RAWINPUT_STANDBY,		TEXT("Standby")},
	{RAWINPUT_OEM1,			TEXT("OEM1")},
	{RAWINPUT_OEM2,			TEXT("OEM2")},
	{RAWINPUT_MYTV,			TEXT("My TV")},
	{RAWINPUT_MYVIDEOS,		TEXT("My Videos")},
	{RAWINPUT_MYPICTURES,	TEXT("My Pictures")},
	{RAWINPUT_MYMUSIC,		TEXT("My Music")},
	{RAWINPUT_RECORDEDTV,	TEXT("Recorded TV")},
	{RAWINPUT_DVDANGLE,		TEXT("DVD Angle")},
	{RAWINPUT_DVDAUDIO,		TEXT("DVD Audio")},
	{RAWINPUT_DVDMENU,		TEXT("DVD Menu")},
	{RAWINPUT_DVDSUBTITLE,	TEXT("DVD Subtitle")},
};




CRawInput::CRawInput()
	: m_pRegisterRawInputDevices(NULL)
	, m_pGetRawInputData(NULL)
	, m_pEventHandler(NULL)
{
}


CRawInput::~CRawInput()
{
}


bool CRawInput::Initialize(HWND hwnd)
{
	if (m_pRegisterRawInputDevices==NULL || m_pGetRawInputData==NULL) {
		HMODULE hLib=::GetModuleHandle(TEXT("user32.dll"));
		if (hLib==NULL)
			return false;

		m_pRegisterRawInputDevices=
			reinterpret_cast<RegisterRawInputDevicesFunc>(::GetProcAddress(hLib,"RegisterRawInputDevices"));
		m_pGetRawInputData=
			reinterpret_cast<GetRawInputDataFunc>(::GetProcAddress(hLib,"GetRawInputData"));
		if (m_pRegisterRawInputDevices==NULL || m_pGetRawInputData==NULL) {
			m_pRegisterRawInputDevices=NULL;
			m_pGetRawInputData=NULL;
			return false;
		}
	}

	RAWINPUTDEVICE rid[2];

	rid[0].usUsagePage=0xFFBC;
	rid[0].usUsage=0x88;
	rid[0].dwFlags=RIDEV_INPUTSINK;
	rid[0].hwndTarget=hwnd;
	rid[1].usUsagePage=0x0C;
	rid[1].usUsage=0x01;
	rid[1].dwFlags=RIDEV_INPUTSINK;
	rid[1].hwndTarget=hwnd;
	return m_pRegisterRawInputDevices(rid,lengthof(rid),sizeof(RAWINPUTDEVICE))!=FALSE;
}


LRESULT CRawInput::OnInput(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	HRAWINPUT hRawInput=reinterpret_cast<HRAWINPUT>(lParam);
	UINT Size=0;
	BYTE *pData;

	if (m_pGetRawInputData==NULL || m_pEventHandler==NULL)
		return 0;
	m_pGetRawInputData(hRawInput,RID_INPUT,NULL,&Size,sizeof(RAWINPUTHEADER));
	if (Size==0)
		return 0;
	pData=new BYTE[Size];
	if (m_pGetRawInputData(hRawInput,RID_INPUT,pData,&Size,sizeof(RAWINPUTHEADER))==Size) {
		RAWINPUT *pri=reinterpret_cast<RAWINPUT*>(pData);

		if (pri->header.dwType==RIM_TYPEHID) {
			if (pri->data.hid.dwCount>=1 && pri->data.hid.dwSizeHid>=3) {
				BYTE *p=pri->data.hid.bRawData;
				int Index=KeyDataToIndex(p[1]|(p[2]<<8));

				TRACE(TEXT("WM_INPUT 0x%02x%02x%02x%02x\n"),p[0],p[1],p[2],p[3]);
				if (Index>=0) {
					m_pEventHandler->OnInput(Index);
				} else {
					m_pEventHandler->OnUnknownInput(p,pri->data.hid.dwCount*pri->data.hid.dwSizeHid);
				}
			}
		}
	}
	delete [] pData;
	return 0;
}


void CRawInput::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


int CRawInput::NumKeyTypes() const
{
	return lengthof(KeyList);
}


LPCTSTR CRawInput::GetKeyText(int Key) const
{
	if (Key<0 || Key>=lengthof(KeyList))
		return NULL;
	return KeyList[Key].pszText;
}


int CRawInput::GetKeyData(int Key) const
{
	if (Key<0 || Key>=lengthof(KeyList))
		return 0;
	return KeyList[Key].RawData;
}


int CRawInput::KeyDataToIndex(int Data) const
{
	for (int i=0;i<lengthof(KeyList);i++) {
		if (KeyList[i].RawData==Data)
			return i;
	}
	return -1;
}
