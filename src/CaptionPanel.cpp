#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CaptionPanel.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define WM_APP_ADD_CAPTION WM_APP

#define IDC_EDIT	1000




const LPCTSTR CCaptionPanel::m_pszClassName=APP_NAME TEXT(" Caption Panel");
const LPCTSTR CCaptionPanel::m_pszPropName=TEXT("CaptionPanel");
HINSTANCE CCaptionPanel::m_hinst=NULL;


bool CCaptionPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CCaptionPanel::CCaptionPanel()
	: m_BackColor(RGB(0,0,0))
	, m_TextColor(RGB(255,255,255))
	, m_hwndEdit(NULL)
	, m_pOldEditProc(NULL)
	, m_fEnable(true)
	, m_fAutoScroll(true)
#ifndef TVH264
	, m_fIgnoreSmall(true)
#endif
	, m_Language(0)
{
}


CCaptionPanel::~CCaptionPanel()
{
	Destroy();
	Clear();
}


bool CCaptionPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("Žš–‹"),m_hinst);
}


void CCaptionPanel::SetVisible(bool fVisible)
{
	if (m_hwnd!=NULL) {
		if (fVisible) {
			CBlockLock Lock(&m_Lock);

			if (!m_CaptionList.empty()) {
				TVTest::String Text;

				for (std::deque<LPTSTR>::iterator itr=m_CaptionList.begin();itr!=m_CaptionList.end();++itr)
					Text+=*itr;
				Text+=m_NextCaption;
				AppendText(Text.c_str());
				ClearCaptionList();
			}
		}
		CPanelForm::CPage::SetVisible(fVisible);
	}
}


void CCaptionPanel::SetColor(COLORREF BackColor,COLORREF TextColor)
{
	m_BackColor=BackColor;
	m_TextColor=TextColor;
	m_BackBrush.Destroy();
	if (m_hwnd!=NULL) {
		m_BackBrush.Create(BackColor);
		::InvalidateRect(m_hwndEdit,NULL,TRUE);
	}
}


bool CCaptionPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	if (m_hwnd!=NULL) {
		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),TRUE);
	}
	return true;
}


void CCaptionPanel::Clear()
{
	CBlockLock Lock(&m_Lock);

	ClearCaptionList();
	if (m_hwndEdit!=NULL) {
		::SetWindowText(m_hwndEdit,TEXT(""));
		m_fClearLast=true;
		m_fContinue=false;
	}
	m_DRCSMap.Reset();
}


bool CCaptionPanel::LoadDRCSMap(LPCTSTR pszFileName)
{
	return m_DRCSMap.Load(pszFileName);
}


void CCaptionPanel::ClearCaptionList()
{
	if (!m_CaptionList.empty()) {
		for (std::deque<LPTSTR>::iterator itr=m_CaptionList.begin();itr!=m_CaptionList.end();++itr)
			delete [] *itr;
		m_CaptionList.clear();
	}

	m_NextCaption.clear();
}


void CCaptionPanel::AppendText(LPCTSTR pszText)
{
	bool fScroll=false;
	DWORD SelStart,SelEnd;

	if (m_fAutoScroll) {
		SCROLLINFO si;
		si.cbSize=sizeof(si);
		si.fMask=SIF_RANGE | SIF_PAGE | SIF_POS;
		::GetScrollInfo(m_hwndEdit,SB_VERT,&si);
		if (si.nPos>=si.nMax-(int)si.nPage)
			fScroll=true;
	}
	::SendMessage(m_hwndEdit,EM_GETSEL,
				  reinterpret_cast<WPARAM>(&SelStart),
				  reinterpret_cast<LPARAM>(&SelEnd));
	::SendMessage(m_hwndEdit,EM_SETSEL,::GetWindowTextLength(m_hwndEdit),-1);
	::SendMessage(m_hwndEdit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(pszText));
	::SendMessage(m_hwndEdit,EM_SETSEL,SelStart,SelEnd);
	if (fScroll)
		::SendMessage(m_hwndEdit,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),0);
}


LRESULT CCaptionPanel::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (!m_BackBrush.IsCreated())
				m_BackBrush.Create(m_BackColor);
			if (!m_Font.IsCreated())
				CreateDefaultFont(&m_Font);

			m_hwndEdit=CreateWindowEx(0,TEXT("EDIT"),TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
				0,0,0,0,hwnd,(HMENU)IDC_EDIT,m_hinst,NULL);
			SetWindowFont(m_hwndEdit,m_Font.GetHandle(),FALSE);
			::SetProp(m_hwndEdit,m_pszPropName,this);
			m_pOldEditProc=SubclassWindow(m_hwndEdit,EditWndProc);

			m_fClearLast=true;
			m_fContinue=false;

			CCaptionDecoder *pCaptionDecoder=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_CaptionDecoder;
			pCaptionDecoder->SetCaptionHandler(this);
			pCaptionDecoder->SetDRCSMap(&m_DRCSMap);
		}
		return 0;

	case WM_SIZE:
		::MoveWindow(m_hwndEdit,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
		return 0;

	case WM_CTLCOLORSTATIC:
		{
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc,m_TextColor);
			::SetBkColor(hdc,m_BackColor);
			return reinterpret_cast<LRESULT>(m_BackBrush.GetHandle());
		}

	case WM_APP_ADD_CAPTION:
		{
			CBlockLock Lock(&m_Lock);

			if (!m_NextCaption.empty()) {
				if (m_fEnable) {
					if (GetVisible()) {
						::SendMessage(m_hwndEdit,WM_SETREDRAW,FALSE,0);
						AppendText(m_NextCaption.c_str());
						::SendMessage(m_hwndEdit,WM_SETREDRAW,TRUE,0);
					} else {
						// ”ñ•\Ž¦‚Ìê‡‚ÍƒLƒ…[‚É—­‚ß‚é
						if (m_CaptionList.size()>=MAX_QUEUE_TEXT) {
							delete [] m_CaptionList.front();
							m_CaptionList.pop_front();
							::SetWindowText(m_hwndEdit,TEXT(""));
						}
						m_CaptionList.push_back(DuplicateString(m_NextCaption.c_str()));
					}
				}
				m_NextCaption.clear();
			}
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_CAPTIONPANEL_COPY:
			{
				HWND hwndEdit=m_hwndEdit;
				DWORD Start,End;

				::SendMessage(hwndEdit,WM_SETREDRAW,FALSE,0);
				::SendMessage(hwndEdit,EM_GETSEL,(WPARAM)&Start,(LPARAM)&End);
				if (Start==End)
					::SendMessage(hwndEdit,EM_SETSEL,0,-1);
				::SendMessage(hwndEdit,WM_COPY,0,0);
				if (Start==End)
					::SendMessage(hwndEdit,EM_SETSEL,Start,End);
				::SendMessage(hwndEdit,WM_SETREDRAW,TRUE,0);
			}
			break;

		case CM_CAPTIONPANEL_SELECTALL:
			::SendMessage(m_hwndEdit,EM_SETSEL,0,-1);
			break;

		case CM_CAPTIONPANEL_CLEAR:
			::SetWindowText(m_hwndEdit,TEXT(""));
			break;

		case CM_CAPTIONPANEL_ENABLE:
			m_Lock.Lock();
			m_fEnable=!m_fEnable;
			m_fClearLast=false;
			m_fContinue=false;
			m_Lock.Unlock();
			break;

		case CM_CAPTIONPANEL_AUTOSCROLL:
			m_fAutoScroll=!m_fAutoScroll;
			break;

#ifndef TVH264_FOR_1SEG
		case CM_CAPTIONPANEL_IGNORESMALL:
			m_Lock.Lock();
			m_fIgnoreSmall=!m_fIgnoreSmall;
			m_Lock.Unlock();
			break;
#endif
		}
		return 0;

	case WM_DESTROY:
		{
			CCaptionDecoder *pCaptionDecoder=&GetAppClass().GetCoreEngine()->m_DtvEngine.m_CaptionDecoder;
			pCaptionDecoder->SetCaptionHandler(NULL);
			pCaptionDecoder->SetDRCSMap(NULL);

			ClearCaptionList();
			m_hwndEdit=NULL;
			m_pOldEditProc=NULL;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


LRESULT CALLBACK CCaptionPanel::EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CCaptionPanel *pThis=static_cast<CCaptionPanel*>(::GetProp(hwnd,m_pszPropName));

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_RBUTTONDOWN:
		{
			HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDM_CAPTIONPANEL));

			::CheckMenuItem(hmenu,CM_CAPTIONPANEL_ENABLE,
							MF_BYCOMMAND | (pThis->m_fEnable?MFS_CHECKED:MFS_UNCHECKED));
			::CheckMenuItem(hmenu,CM_CAPTIONPANEL_AUTOSCROLL,
							MF_BYCOMMAND | (pThis->m_fAutoScroll?MFS_CHECKED:MFS_UNCHECKED));
#ifndef TVH264
			::CheckMenuItem(hmenu,CM_CAPTIONPANEL_IGNORESMALL,
							MF_BYCOMMAND | (pThis->m_fIgnoreSmall?MFS_CHECKED:MFS_UNCHECKED));
#endif
			POINT pt;
			::GetCursorPos(&pt);
			::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,0,pThis->m_hwnd,NULL);
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_RBUTTONUP:
		return 0;

	case WM_NCDESTROY:
		SubclassWindow(hwnd,pThis->m_pOldEditProc);
		::RemoveProp(hwnd,m_pszPropName);
		pThis->m_hwndEdit=NULL;
		break;
	}

	return ::CallWindowProc(pThis->m_pOldEditProc,hwnd,uMsg,wParam,lParam);
}


void CCaptionPanel::OnLanguageUpdate(CCaptionDecoder *pDecoder)
{
}


void CCaptionPanel::OnCaption(CCaptionDecoder *pDecoder,BYTE Language, LPCTSTR pszText,const CAribString::FormatList *pFormatList)
{
	CBlockLock Lock(&m_Lock);

	if ((Language==m_Language || Language==0xFF) && m_hwnd!=NULL && m_fEnable) {
		int Length=::lstrlen(pszText);

		if (Length>0) {
			int i;
			for (i=0;i<Length;i++) {
				if (pszText[i]!='\f')
					break;
			}
			if (i==Length) {
				if (m_fClearLast || m_fContinue)
					return;
				m_fClearLast=true;
				m_NextCaption+=TEXT("\r\n");
				::PostMessage(m_hwnd,WM_APP_ADD_CAPTION,0,0);
				return;
			} else {
				m_fClearLast=false;
			}

			LPTSTR pszBuff=new TCHAR[Length+2];
			::lstrcpy(pszBuff,pszText);
			DWORD DstLength=Length;

#ifndef TVH264
			if (m_fIgnoreSmall) {
				for (int i=(int)pFormatList->size()-1;i>=0;i--) {
					if ((*pFormatList)[i].Size==CAribString::SIZE_SMALL) {
						DWORD Pos=(*pFormatList)[i].Pos;
						if (Pos<DstLength) {
							if (i+1<(int)pFormatList->size()) {
								DWORD NextPos=min(DstLength,(*pFormatList)[i+1].Pos);
#ifdef _DEBUG
								TCHAR szTrace[1024];
								::lstrcpyn(szTrace,&pszBuff[Pos],NextPos-Pos+1);
								TRACE(TEXT("Caption exclude : %s\n"),szTrace);
#endif
								memmove(&pszBuff[Pos],&pszBuff[NextPos],
										(DstLength-NextPos+1)*sizeof(TCHAR));
								DstLength-=NextPos-Pos;
							} else {
								pszBuff[Pos]='\0';
								DstLength=Pos;
							}
						}
					}
				}
			}
#endif

			for (DWORD i=0;i<DstLength;i++) {
				if (pszBuff[i]=='\f') {
					if (i==0 && !m_fContinue) {
						memmove(&pszBuff[2],&pszBuff[1],DstLength*sizeof(TCHAR));
						pszBuff[0]='\r';
						pszBuff[1]='\n';
						i++;
						DstLength++;
					} else {
						memmove(&pszBuff[i],&pszBuff[i+1],(DstLength-i)*sizeof(TCHAR));
						DstLength--;
					}
				}
			}
			m_fContinue=
#ifdef UNICODE
				DstLength>1 && pszBuff[DstLength-1]==L'¨';
#else
				DstLength>2 && pszBuff[DstLength-2]=="¨"[0] && pszBuff[DstLength-1]=="¨"[1];
#endif
			if (m_fContinue)
				pszBuff[DstLength-(3-sizeof(TCHAR))]='\0';
			if (DstLength>0) {
				m_NextCaption+=pszBuff;
				::PostMessage(m_hwnd,WM_APP_ADD_CAPTION,0,0);
			}
			delete [] pszBuff;
		}
	}
}




#include "Settings.h"
#include "BonTsEngine/TsUtilClass.h"	// For MD5


CCaptionDRCSMap::CCaptionDRCSMap()
	: m_pBuffer(NULL)
	, m_fSaveBMP(false)
	, m_fSaveRaw(false)
{
	GetAppClass().GetAppDirectory(m_szSaveDirectory);
	::PathAppend(m_szSaveDirectory,TEXT("DRCS"));
}


CCaptionDRCSMap::~CCaptionDRCSMap()
{
	delete [] m_pBuffer;
}


void CCaptionDRCSMap::Clear()
{
	CBlockLock Lock(&m_Lock);

	m_HashMap.clear();
	m_CodeMap.clear();
	if (m_pBuffer!=NULL) {
		delete [] m_pBuffer;
		m_pBuffer=NULL;
	}
}


void CCaptionDRCSMap::Reset()
{
	CBlockLock Lock(&m_Lock);

	m_CodeMap.clear();
}


bool CCaptionDRCSMap::Load(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	Clear();

	CSettings Settings;
	if (Settings.Open(pszFileName,CSettings::OPEN_READ)) {
		if (Settings.SetSection(TEXT("Settings"))) {
			Settings.Read(TEXT("SaveBMP"),&m_fSaveBMP);
			Settings.Read(TEXT("SaveRaw"),&m_fSaveRaw);
			TCHAR szDir[MAX_PATH];
			if (Settings.Read(TEXT("SaveDirectory"),szDir,MAX_PATH) && szDir[0]!='\0') {
				if (::PathIsRelative(szDir)) {
					TCHAR szTmp[MAX_PATH];
					GetAppClass().GetAppDirectory(szTmp);
					::PathAppend(szTmp,szDir);
					::PathCanonicalize(m_szSaveDirectory,szTmp);
				} else {
					::lstrcpy(m_szSaveDirectory,szDir);
				}
			}
		}
		Settings.Close();
	}

	const DWORD BuffLength=32767;
	LPTSTR pBuffer=new TCHAR[BuffLength];
	if (pBuffer==NULL)
		return false;
	DWORD Length=::GetPrivateProfileSection(TEXT("DRCSMap"),pBuffer,BuffLength,pszFileName);
	if (Length==0) {
		delete [] pBuffer;
		return false;
	}
	for (LPTSTR p=pBuffer;*p!='\0';p+=::lstrlen(p)+1) {
		BYTE Hash[16],v;
		int i;
		for (i=0;i<32;i++) {
			if (p[i]>='0' && p[i]<='9')
				v=p[i]-'0';
			else if (p[i]>='A' && p[i]<='F')
				v=p[i]-'A'+10;
			else if (p[i]>='a' && p[i]<='f')
				v=p[i]-'a'+10;
			else
				break;
			v<<=4;
			i++;
			if (p[i]>='0' && p[i]<='9')
				v|=p[i]-'0';
			else if (p[i]>='A' && p[i]<='F')
				v|=p[i]-'A'+10;
			else if (p[i]>='a' && p[i]<='f')
				v|=p[i]-'a'+10;
			else
				break;
			Hash[i/2]=v;
		}
		if (i==32 && p[i]=='=') {
			::CopyMemory(p,Hash,sizeof(Hash));
			m_HashMap.insert(std::pair<PBYTE,LPCTSTR>((PBYTE)p,&p[i+1]));
			/*
			TRACE(TEXT("DRCS map : %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X = %s\n"),
				  Hash[0],Hash[1],Hash[2],Hash[3],Hash[4],Hash[5],Hash[6],Hash[7],
				  Hash[8],Hash[9],Hash[10],Hash[11],Hash[12],Hash[13],Hash[14],Hash[15],
				  &p[i+1]);
			*/
		}
	}
	if (m_HashMap.size()==0)
		delete [] pBuffer;
	else
		m_pBuffer=pBuffer;
	return true;
}


LPCTSTR CCaptionDRCSMap::GetString(WORD Code)
{
	CBlockLock Lock(&m_Lock);
	CodeMap::iterator itr=m_CodeMap.find(Code);

	if (itr!=m_CodeMap.end()) {
		TRACE(TEXT("DRCS : Code %d %s\n"),Code,itr->second);
		return itr->second;
	}
	return NULL;
}


static void MakeSaveFileName(const BYTE *pMD5,LPTSTR pszFileName,LPCTSTR pszExtension)
{
	static const TCHAR Hex[] = TEXT("0123456789ABCDEF");
	for (int i=0;i<16;i++) {
		pszFileName[i*2+0]=Hex[pMD5[i]>>4];
		pszFileName[i*2+1]=Hex[pMD5[i]&0x0F];
	}
	::lstrcpy(pszFileName+32,pszExtension);
}

bool CCaptionDRCSMap::SetDRCS(WORD Code, const CCaptionParser::DRCSBitmap *pBitmap)
{
	CBlockLock Lock(&m_Lock);
	BYTE MD5[16];

	CMD5Calculator::CalcMD5(pBitmap->pBits,pBitmap->BitsSize,MD5);
	TRACE(TEXT("DRCS : Code %d, %d x %d (%d), MD5 %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
		  Code,pBitmap->Width,pBitmap->Height,pBitmap->Depth,
		  MD5[0],MD5[1],MD5[2],MD5[3],MD5[4],MD5[5],MD5[6],MD5[7],
		  MD5[8],MD5[9],MD5[10],MD5[11],MD5[12],MD5[13],MD5[14],MD5[15]);
	HashMap::iterator itr=m_HashMap.find(MD5);
	if (itr!=m_HashMap.end()) {
		TRACE(TEXT("DRCS assign %d = %s\n"),Code,itr->second);
		if (!m_CodeMap.insert(std::pair<WORD,LPCTSTR>(Code,itr->second)).second)
			m_CodeMap[Code]=itr->second;
	}
	if (m_fSaveBMP) {
		TCHAR szFileName[40],szFilePath[MAX_PATH];
		MakeSaveFileName(MD5,szFileName,TEXT(".bmp"));
		::PathCombine(szFilePath,m_szSaveDirectory,szFileName);
		if (!::PathFileExists(szFilePath))
			SaveBMP(pBitmap,szFilePath);
	}
	if (m_fSaveRaw) {
		TCHAR szFileName[40],szFilePath[MAX_PATH];
		MakeSaveFileName(MD5,szFileName,TEXT(".drcs"));
		::PathCombine(szFilePath,m_szSaveDirectory,szFileName);
		if (!::PathFileExists(szFilePath))
			SaveRaw(pBitmap,szFilePath);
	}
	return true;
}


bool CCaptionDRCSMap::SaveBMP(const CCaptionParser::DRCSBitmap *pBitmap,LPCTSTR pszFileName)
{
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
							  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;

	DWORD Write;
	const int BitCount=pBitmap->BitsPerPixel==1?1:8;
	const DWORD DIBRowBytes=(pBitmap->Width*BitCount+31)/32*4;
	const DWORD BitsSize=DIBRowBytes*pBitmap->Height;
	BITMAPFILEHEADER bmfh;
	bmfh.bfType=0x4D42;
	bmfh.bfOffBits=sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+(1UL<<BitCount)*(DWORD)sizeof(RGBQUAD);
	bmfh.bfSize=bmfh.bfOffBits+BitsSize;
	bmfh.bfReserved1=0;
	bmfh.bfReserved2=0;
	if (!::WriteFile(hFile,&bmfh,sizeof(bmfh),&Write,NULL) || Write!=sizeof(bmfh)) {
		::CloseHandle(hFile);
		return false;
	}

	BITMAPINFOHEADER bmih;
	bmih.biSize=sizeof(BITMAPINFOHEADER);
	bmih.biWidth=pBitmap->Width;
	bmih.biHeight=pBitmap->Height;
	bmih.biPlanes=1;
	bmih.biBitCount=BitCount;
	bmih.biCompression=BI_RGB;
	bmih.biSizeImage=0;
	bmih.biXPelsPerMeter=0;
	bmih.biYPelsPerMeter=0;
	bmih.biClrUsed=0;
	bmih.biClrImportant=0;
	if (!::WriteFile(hFile,&bmih,sizeof(bmih),&Write,NULL) || Write!=sizeof(bmih)) {
		::CloseHandle(hFile);
		return false;
	}

	RGBQUAD Colormap[256];
	for (int i=0;i<1<<BitCount;i++) {
		BYTE v=(BYTE)(i*255/((1<<BitCount)-1));
		Colormap[i].rgbBlue=v;
		Colormap[i].rgbGreen=v;
		Colormap[i].rgbRed=v;
		Colormap[i].rgbReserved=0;
	}
	DWORD PalSize=(1UL<<BitCount)*(DWORD)sizeof(RGBQUAD);
	if (!::WriteFile(hFile,Colormap,PalSize,&Write,NULL) || Write!=PalSize) {
		::CloseHandle(hFile);
		return false;
	}

	BYTE *pDIBBits=new BYTE[BitsSize];
	const BYTE *p=static_cast<const BYTE*>(pBitmap->pBits);
	BYTE *q=pDIBBits+(pBitmap->Height-1)*DIBRowBytes;
	int x,y;
	if (BitCount==1) {
		BYTE Mask;

		::ZeroMemory(pDIBBits,BitsSize);
		Mask=0x80;
		for (y=0;y<pBitmap->Height;y++) {
			for (x=0;x<pBitmap->Width;x++) {
				if ((*p&Mask)!=0)
					q[x>>3]|=0x80>>(x&7);
				Mask>>=1;
				if (Mask==0) {
					Mask=0x80;
					p++;
				}
			}
			q-=DIBRowBytes;
		}
	} else {
		int Shift;
		unsigned int Mask,Pixel,Max=pBitmap->Depth+1;

		Shift=16-pBitmap->BitsPerPixel;
		Mask=(1<<pBitmap->BitsPerPixel)-1;
		for (y=0;y<pBitmap->Height;y++) {
			for (x=0;x<pBitmap->Width;x++) {
				Pixel=*p;
				if (Shift<8)
					Pixel=(Pixel<<(8-Shift))|(*p>>Shift);
				else
					Pixel>>=Shift-8;
				Pixel=(Pixel&Mask)*255/Max;
				q[x]=(BYTE)min(Pixel,255);
				Shift-=pBitmap->BitsPerPixel;
				if (Shift<0) {
					Shift+=16;
					p++;
				}
				if (Shift+pBitmap->BitsPerPixel<=8) {
					Shift+=8;
					p++;
				}
			}
			q-=DIBRowBytes;
		}
	}
	if (!::WriteFile(hFile,pDIBBits,BitsSize,&Write,NULL) || Write!=BitsSize) {
		delete [] pDIBBits;
		::CloseHandle(hFile);
		return false;
	}
	delete [] pDIBBits;

	::CloseHandle(hFile);
	return true;
}


bool CCaptionDRCSMap::SaveRaw(const CCaptionParser::DRCSBitmap *pBitmap,LPCTSTR pszFileName)
{
	HANDLE hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
							  CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	DWORD Write;
	if (!::WriteFile(hFile,pBitmap->pBits,pBitmap->BitsSize,&Write,NULL)
			|| Write!=pBitmap->BitsSize) {
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	return true;
}
