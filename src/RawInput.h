#ifndef TVTEST_RAW_INPUT_H
#define TVTEST_RAW_INPUT_H


namespace TVTest
{

	class CRawInput
	{
	public:
		class CEventHandler
		{
		public:
			virtual ~CEventHandler() = default;

			virtual void OnInput(int Type) = 0;
			virtual void OnUnknownInput(const BYTE *pData, int Size) {}
		};

	protected:
		typedef BOOL (WINAPI *RegisterRawInputDevicesFunc)(PCRAWINPUTDEVICE pRawInputDevices, UINT uiNumDevices, UINT cbSize);
		typedef UINT (WINAPI *GetRawInputDataFunc)(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader);

		RegisterRawInputDevicesFunc m_pRegisterRawInputDevices;
		GetRawInputDataFunc m_pGetRawInputData;
		CEventHandler *m_pEventHandler;

	public:
		CRawInput();
		~CRawInput();

		bool Initialize(HWND hwnd);
		LRESULT OnInput(HWND hwnd, WPARAM wParam, LPARAM lParam);
		void SetEventHandler(CEventHandler *pHandler);
		CEventHandler *GetEventHandler() const { return m_pEventHandler; }
		int NumKeyTypes() const;
		LPCTSTR GetKeyText(int Key) const;
		int GetKeyData(int Key) const;
		int KeyDataToIndex(int Data) const;
	};

}	// namespace TVTest


#endif
