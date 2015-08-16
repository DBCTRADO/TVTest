/*
	TVTest �v���O�C���T���v��

	�X�y�N�g�����E�A�i���C�U�[

	fft4g.c �� http://www.kurims.kyoto-u.ac.jp/~ooura/fft-j.html �Ŕz�z�����
	������̂ŁA��Y��Ǝ��ɒ��쌠������܂��B
	Copyright Takuya OOURA, 1996-2001
*/


#include <windows.h>
#include <gdiplus.h>
#include <shlwapi.h>
#define _USE_MATH_DEFINES
#include <cmath>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// �N���X�Ƃ��Ď���
#include "TVTestPlugin.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")


// fft4g.c
extern "C" void rdft(int n, int isgn, double *a, int *ip, double *w);


// FFT �������s���T���v����
#define FFT_SIZE_BITS 10
#define FFT_SIZE (1 << FFT_SIZE_BITS)
// �ш敪����
#define NUM_BANDS 20
// ���x��������
#define NUM_LEVELS 30


// �v���O�C���N���X
class CSpectrumAnalyzer : public TVTest::CTVTestPlugin
{
public:
	CSpectrumAnalyzer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

private:
	struct Position
	{
		int Left, Top, Width, Height;
		Position() : Left(0), Top(0), Width(0), Height(0) {}
	};

	class CCriticalLock
	{
	public:
		CCriticalLock() {
			::InitializeCriticalSection(&m_CriticalSection);
		}
		virtual ~CCriticalLock() {
			::DeleteCriticalSection(&m_CriticalSection);
		}
		void Lock() {
			::EnterCriticalSection(&m_CriticalSection);
		}
		void Unlock() {
			::LeaveCriticalSection(&m_CriticalSection);
		}

	private:
		CRITICAL_SECTION m_CriticalSection;
	};

	bool m_fInitialized;
	HWND m_hwnd;
	Position m_WindowPosition;
	Gdiplus::Color m_BackColor;
	Gdiplus::Color m_SpectrumColor1;
	Gdiplus::Color m_SpectrumColor2;
	Gdiplus::SolidBrush *m_pBrush;
	Gdiplus::Graphics *m_pOffscreen;
	Gdiplus::Bitmap *m_pOffscreenImage;

	short *m_pSampleBuffer;
	double *m_pFFTBuffer;
	int *m_pFFTWorkBuffer;
	double *m_pFFTSinTable;
	DWORD m_BufferUsed;
	DWORD m_BufferPos;
	double *m_pPower;
	struct {
		double Real;
		double Delayed;
	} m_PeakList[NUM_BANDS];
	CCriticalLock m_FFTLock;

	static const LPCTSTR WINDOW_CLASS_NAME;

	bool EnablePlugin(bool fEnable);
	void DrawSpectrum(Gdiplus::Graphics &Graphics, int Width, int Height);
	void OnPaint(HWND hwnd);
	void UpdateSpectrum();

	static LRESULT CALLBACK EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData);
	static CSpectrumAnalyzer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	template<typename T> static inline void SafeDelete(T *&p)
	{
		if (p != nullptr) {
			delete p;
			p = nullptr;
		}
	}

	template<typename T> static inline void SafeDeleteArray(T *&p)
	{
		if (p != nullptr) {
			delete [] p;
			p = nullptr;
		}
	}

	static inline double Pow2(double v) { return std::pow(v, 2.0); }
};


// �E�B���h�E�N���X��
const LPCTSTR CSpectrumAnalyzer::WINDOW_CLASS_NAME = TEXT("TVTest Spectrum Analyzer Window");


CSpectrumAnalyzer::CSpectrumAnalyzer()
	: m_fInitialized(false)
	, m_hwnd(nullptr)
	, m_BackColor(255, 0, 0, 0)
	, m_SpectrumColor1(255, 0, 224, 0)
	, m_SpectrumColor2(255, 255, 128, 0)
	, m_pBrush(nullptr)
	, m_pOffscreen(nullptr)
	, m_pOffscreenImage(nullptr)
	, m_pSampleBuffer(nullptr)
	, m_pFFTBuffer(nullptr)
	, m_pFFTWorkBuffer(nullptr)
	, m_pFFTSinTable(nullptr)
	, m_BufferUsed(0)
	, m_BufferPos(0)
	, m_pPower(nullptr)
{
}


// �v���O�C���̏���Ԃ�
bool CSpectrumAnalyzer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Spectrum Analyzer";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�X�y�N�g�����E�A�i���C�U�[";
	return true;
}


// ����������
bool CSpectrumAnalyzer::Initialize()
{
	// �A�C�R����o�^
	m_pApp->RegisterPluginIconFromResource(g_hinstDLL, MAKEINTRESOURCE(IDB_ICON));

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


// �I������
bool CSpectrumAnalyzer::Finalize()
{
	// �E�B���h�E���쐬����Ă�����j������
	if (m_hwnd != nullptr)
		::DestroyWindow(m_hwnd);

	// �ݒ��ۑ�
	if (m_fInitialized) {
		TCHAR szIniFileName[MAX_PATH];

		::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
		::PathRenameExtension(szIniFileName, TEXT(".ini"));

		struct IntString {
			IntString(int Value) { ::wsprintf(m_szBuffer, TEXT("%d"), Value); }
			operator LPCTSTR() const { return m_szBuffer; }
			TCHAR m_szBuffer[16];
		};

		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowLeft"),
									IntString(m_WindowPosition.Left), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowTop"),
									IntString(m_WindowPosition.Top), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowWidth"),
									IntString(m_WindowPosition.Width), szIniFileName);
		::WritePrivateProfileString(TEXT("Settings"), TEXT("WindowHeight"),
									IntString(m_WindowPosition.Height), szIniFileName);
	}

	return true;
}


// �v���O�C���̗L��/�����̐؂�ւ�
bool CSpectrumAnalyzer::EnablePlugin(bool fEnable)
{
	if (fEnable) {
		if (!m_fInitialized) {
			// �E�B���h�E�N���X�o�^
			WNDCLASS wc;
			wc.style = CS_HREDRAW | CS_VREDRAW;
			wc.lpfnWndProc = WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = 0;
			wc.hInstance = g_hinstDLL;
			wc.hIcon = nullptr;
			wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
			wc.hbrBackground = nullptr;
			wc.lpszMenuName = nullptr;
			wc.lpszClassName = WINDOW_CLASS_NAME;
			if (::RegisterClass(&wc) == 0)
				return false;

			// �ݒ�̓ǂݍ���
			TCHAR szIniFileName[MAX_PATH];
			::GetModuleFileName(g_hinstDLL, szIniFileName, _countof(szIniFileName));
			::PathRenameExtension(szIniFileName, TEXT(".ini"));
			m_WindowPosition.Left =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowLeft"), m_WindowPosition.Left, szIniFileName);
			m_WindowPosition.Top =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowTop"), m_WindowPosition.Top, szIniFileName);
			m_WindowPosition.Width =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowWidth"), m_WindowPosition.Width, szIniFileName);
			m_WindowPosition.Height =
				::GetPrivateProfileInt(TEXT("Settings"), TEXT("WindowHeight"), m_WindowPosition.Height, szIniFileName);

			m_fInitialized = true;
		}

		// �E�B���h�E���쐬����
		if (m_hwnd == nullptr) {
			static const DWORD Style = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
			static const DWORD ExStyle = WS_EX_TOOLWINDOW;

			// �f�t�H���g�̃E�B���h�E�T�C�Y���擾
			if (m_WindowPosition.Width <= 0 || m_WindowPosition.Height <= 0) {
				int Width = NUM_BANDS * 10, Height = NUM_LEVELS * 4;
				// DPI�ݒ�ɍ��킹�ăX�P�[�����O
				int DPI;
				if (m_pApp->GetSetting(L"DPI", &DPI) && DPI != 96) {
					Width = ::MulDiv(Width, DPI, 96);
					Height = ::MulDiv(Height, DPI, 96);
				}
				RECT rc = {0, 0, Width, Height};
				::AdjustWindowRectEx(&rc, Style, FALSE, ExStyle);
				if (m_WindowPosition.Width <= 0)
					m_WindowPosition.Width = rc.right - rc.left;
				if (m_WindowPosition.Height <= 0)
					m_WindowPosition.Height = rc.bottom - rc.top;
			}

			// �E�B���h�E�̍쐬
			if (::CreateWindowEx(
					ExStyle, WINDOW_CLASS_NAME, TEXT("Spectrum Analyzer"), Style,
					0, 0, m_WindowPosition.Width, m_WindowPosition.Height,
					m_pApp->GetAppWindow(), nullptr, g_hinstDLL, this) == nullptr)
				return false;
		}

		WINDOWPLACEMENT wp;
		wp.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(m_hwnd, &wp);
		wp.flags = 0;
		wp.showCmd = SW_SHOWNOACTIVATE;
		wp.rcNormalPosition.left = m_WindowPosition.Left;
		wp.rcNormalPosition.top = m_WindowPosition.Top;
		wp.rcNormalPosition.right = m_WindowPosition.Left + m_WindowPosition.Width;
		wp.rcNormalPosition.bottom = m_WindowPosition.Top + m_WindowPosition.Height;
		::SetWindowPlacement(m_hwnd, &wp);
		::UpdateWindow(m_hwnd);
	} else {
		if (m_hwnd != nullptr)
			::DestroyWindow(m_hwnd);
	}

	return true;
}


// �X�y�N�g������`��
void CSpectrumAnalyzer::DrawSpectrum(Gdiplus::Graphics &Graphics, int Width, int Height)
{
	Graphics.Clear(m_BackColor);

	const int BarWidth = max(Width / NUM_BANDS, 4);
	const int CellHeight = max(Height / NUM_LEVELS, 3);
	const int BaseX = (Width - BarWidth * NUM_BANDS) / 2;
	const int BaseY = (Height - CellHeight * NUM_LEVELS) / 2;

	if (m_pBrush == nullptr)
		m_pBrush = new Gdiplus::SolidBrush(m_SpectrumColor1);

	for (int i = 0; i < NUM_BANDS; i++) {
		int Level = (int)(m_PeakList[i].Delayed * ((double)NUM_LEVELS / 40.0) + 0.5);
		if (Level > 0) {
			if (Level > NUM_LEVELS)
				Level = NUM_LEVELS;

			Gdiplus::Rect Rect;
			Rect.X = BaseX + i * BarWidth + 1;
			Rect.Y = BaseY + Height - CellHeight + 1;
			Rect.Width = BarWidth - 2;
			Rect.Height = CellHeight - 2;

			for (int j = 0; j < Level; j++) {
				const int Ratio = NUM_LEVELS - j;
				int A = ((m_SpectrumColor1.GetA() * Ratio) + (m_SpectrumColor2.GetA() * j)) / NUM_LEVELS;
				m_pBrush->SetColor(
					Gdiplus::Color(
						A,
						((m_SpectrumColor1.GetR() * Ratio) + (m_SpectrumColor2.GetR() * j)) / NUM_LEVELS,
						((m_SpectrumColor1.GetG() * Ratio) + (m_SpectrumColor2.GetG() * j)) / NUM_LEVELS,
						((m_SpectrumColor1.GetB() * Ratio) + (m_SpectrumColor2.GetB() * j)) / NUM_LEVELS));
				Graphics.FillRectangle(m_pBrush, Rect);
				Rect.Y -= CellHeight;
			}
		}
	}
}


// WM_PAINT �̏���
void CSpectrumAnalyzer::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	RECT rcClient;

	::BeginPaint(hwnd, &ps);
	::GetClientRect(hwnd, &rcClient);

	if (m_pOffscreenImage == nullptr
			|| m_pOffscreenImage->GetWidth() < (UINT)rcClient.right
			|| m_pOffscreenImage->GetHeight() < (UINT)rcClient.bottom) {
		SafeDelete(m_pOffscreen);
		delete m_pOffscreenImage;
		m_pOffscreenImage = new Gdiplus::Bitmap(rcClient.right, rcClient.bottom, PixelFormat32bppPARGB);
		if (m_pOffscreenImage != nullptr)
			m_pOffscreen = new Gdiplus::Graphics(m_pOffscreenImage);
	}

	const Gdiplus::Rect PaintRect(
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top);

	if (m_pOffscreen != nullptr) {
		m_pOffscreen->SetPageUnit(Gdiplus::UnitPixel);
		m_pOffscreen->SetClip(PaintRect);

		DrawSpectrum(*m_pOffscreen, rcClient.right, rcClient.bottom);

		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
		Graphics.DrawImage(
			m_pOffscreenImage,
			PaintRect.X, PaintRect.Y,
			PaintRect.X, PaintRect.Y,
			PaintRect.Width, PaintRect.Height,
			Gdiplus::UnitPixel);
	} else {
		Gdiplus::Graphics Graphics(ps.hdc);

		Graphics.SetClip(PaintRect);
		DrawSpectrum(Graphics, rcClient.right, rcClient.bottom);
	}

	::EndPaint(hwnd, &ps);
}


// �X�y�N�g�����X�V
void CSpectrumAnalyzer::UpdateSpectrum()
{
	m_FFTLock.Lock();
	m_pPower[0] = Pow2(m_pFFTBuffer[0]);
	for (int i = 1; i < FFT_SIZE / 2; i++) {
		m_pPower[i] = Pow2(m_pFFTBuffer[i * 2]) +
		              Pow2(m_pFFTBuffer[i * 2 + 1]);
	}
	m_pPower[FFT_SIZE / 2] = Pow2(m_pFFTBuffer[1]);
	m_FFTLock.Unlock();

	int j1 = 0;
	for (int i = 0; i < NUM_BANDS; i++) {
		double Peak = 0.0;
		int j2 = (int)std::pow(2.0, (double)i * (double)(FFT_SIZE_BITS - 2) / (double)(NUM_BANDS - 1));
		/*
		if (j2 > FFT_SIZE / 2)
			j2 = FFT_SIZE / 2;
		else
		*/
		if (j2 <= j1)
			j2 = j1 + 1;
		for (; j1 < j2; j1++) {
			if (m_pPower[j1] > Peak)
				Peak = m_pPower[j1];
		}
		Peak = std::log10(Peak) * 10.0;
		m_PeakList[i].Real = Peak;
		if (Peak < m_PeakList[i].Delayed)
			Peak = max(m_PeakList[i].Delayed - 2.5, Peak);
		m_PeakList[i].Delayed = Peak;
	}
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CSpectrumAnalyzer::EventCallback(UINT Event, LPARAM lParam1, LPARAM lParam2, void *pClientData)
{
	CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		pThis->EnablePlugin(lParam1 != 0);
		return TRUE;

	case TVTest::EVENT_STANDBY:
		// �ҋ@��Ԃ��ω�����
		if (pThis->m_pApp->IsPluginEnabled()) {
			// �ҋ@��Ԃ̎��̓E�B���h�E���B��
			::ShowWindow(pThis->m_hwnd, lParam1 != 0 ? SW_HIDE : SW_SHOW);
		}
		return TRUE;
	}

	return 0;
}


// �����R�[���o�b�N�֐�
LRESULT CALLBACK CSpectrumAnalyzer::AudioCallback(short *pData, DWORD Samples, int Channels, void *pClientData)
{
	CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer*>(pClientData);

	DWORD BufferUsed = pThis->m_BufferUsed;
	DWORD j = pThis->m_BufferPos;
	for (DWORD i = 0; i < Samples; i++) {
		if (j == FFT_SIZE)
			j = 0;
		switch (Channels) {
		case 2:
		case 6:
			pThis->m_pSampleBuffer[j] = (short)((pData[i * Channels] + pData[i * Channels + 1]) / 2);
			break;
		default:
			pThis->m_pSampleBuffer[j] = pData[i * Channels];
			break;
		}
		j++;
		BufferUsed++;

		if (BufferUsed == FFT_SIZE) {
			pThis->m_FFTLock.Lock();
			// double�ɕϊ�
			for (int k = 0; k < FFT_SIZE; k++) {
				pThis->m_pFFTBuffer[k]=
					(double)pThis->m_pSampleBuffer[(j + k) % FFT_SIZE] / 32768.0;
			}

			// FFT�������s
			rdft(FFT_SIZE, 1, pThis->m_pFFTBuffer,
				 pThis->m_pFFTWorkBuffer, pThis->m_pFFTSinTable);
			pThis->m_FFTLock.Unlock();

			BufferUsed = 0;
		}
	}

	pThis->m_BufferUsed = BufferUsed;
	pThis->m_BufferPos = j;

	return 0;
}


CSpectrumAnalyzer *CSpectrumAnalyzer::GetThis(HWND hwnd)
{
	// �E�B���h�E�n���h������this���擾
	return reinterpret_cast<CSpectrumAnalyzer*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
}


LRESULT CALLBACK CSpectrumAnalyzer::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSpectrumAnalyzer *pThis = static_cast<CSpectrumAnalyzer*>(pcs->lpCreateParams);

			// �E�B���h�E�n���h������this���擾�ł���悤�ɂ���
			::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;

			// �������m��
			pThis->m_pSampleBuffer = new short[FFT_SIZE];
			pThis->m_pFFTBuffer = new double[FFT_SIZE];
			for (int i = 0; i < FFT_SIZE; i++)
				pThis->m_pFFTBuffer[i] = 0.0;
			pThis->m_pFFTWorkBuffer = new int[2 + (int)std::sqrt((double)(FFT_SIZE / 2)) + 10];
			pThis->m_pFFTWorkBuffer[0] = 0;
			pThis->m_pFFTSinTable = new double[FFT_SIZE / 2];
			pThis->m_pPower = new double[FFT_SIZE / 2 + 1];

			for (int i = 0; i < NUM_BANDS; i++) {
				pThis->m_PeakList[i].Real = 0.0;
				pThis->m_PeakList[i].Delayed = 0.0;
			}

			pThis->m_BufferUsed = 0;
			pThis->m_BufferPos = 0;

			// �����R�[���o�b�N��o�^
			pThis->m_pApp->SetAudioCallback(AudioCallback, pThis);

			// �\���X�V�p�^�C�}�̐ݒ�
			::SetTimer(hwnd, 1, 200, nullptr);
		}
		return TRUE;

	case WM_PAINT:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			pThis->OnPaint(hwnd);
		}
		return 0;

	case WM_TIMER:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			pThis->UpdateSpectrum();
			::InvalidateRect(hwnd, nullptr, FALSE);
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam & 0xFFF0) == SC_CLOSE) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSpectrumAnalyzer *pThis = GetThis(hwnd);

			// �����R�[���o�b�N��o�^����
			pThis->m_pApp->SetAudioCallback(nullptr);

			// ���\�[�X���
			SafeDelete(pThis->m_pBrush);
			SafeDelete(pThis->m_pOffscreen);
			SafeDelete(pThis->m_pOffscreenImage);

			// �������J��
			SafeDeleteArray(pThis->m_pSampleBuffer);
			SafeDeleteArray(pThis->m_pFFTBuffer);
			SafeDeleteArray(pThis->m_pFFTWorkBuffer);
			SafeDeleteArray(pThis->m_pFFTSinTable);
			SafeDeleteArray(pThis->m_pPower);

			// �E�B���h�E�ʒu�ۑ�
			WINDOWPLACEMENT wp;
			wp.length = sizeof (WINDOWPLACEMENT);
			if (::GetWindowPlacement(hwnd, &wp)) {
				pThis->m_WindowPosition.Left = wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Top = wp.rcNormalPosition.top;
				pThis->m_WindowPosition.Width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
				pThis->m_WindowPosition.Height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
			}

			pThis->m_hwnd = nullptr;
		}
		return 0;
	}

	return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSpectrumAnalyzer;
}
