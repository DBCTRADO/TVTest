#pragma once


struct IEVRSchedulerCallback
{
	virtual HRESULT PresentSample(IMFSample *pSample, LONGLONG llTarget) = 0;
};

class CEVRScheduler
{
public:
	CEVRScheduler();
	virtual ~CEVRScheduler();

	void SetCallback(IEVRSchedulerCallback *pCallback) { m_pCallback = pCallback; }

	void SetFrameRate(const MFRatio &fps);
	void SetClockRate(float Rate) { m_Rate = Rate; }

	LONGLONG LastSampleTime() const { return m_LastSampleTime; }
	LONGLONG FrameDuration() const { return m_PerFrameInterval; }

	HRESULT StartScheduler(IMFClock *pClock);
	HRESULT StopScheduler();

	HRESULT ScheduleSample(IMFSample *pSample, bool bPresentNow);
	HRESULT ProcessSamplesInQueue(LONG *pNextSleep);
	HRESULT ProcessSample(IMFSample *pSample, LONG *pNextSleep);
	HRESULT Flush();

private:
	static unsigned int __stdcall SchedulerThreadProc(void *pParameter);

	unsigned int SchedulerThread();

	ThreadSafeQueue<IMFSample> m_ScheduledSamples;

	IMFClock *m_pClock;
	IEVRSchedulerCallback *m_pCallback;

	DWORD m_ThreadID;
	HANDLE m_hSchedulerThread;
	HANDLE m_hThreadReadyEvent;
	HANDLE m_hFlushEvent;

	float m_Rate;
	MFTIME m_PerFrameInterval;
	LONGLONG m_PerFrame_1_4th;
	MFTIME m_LastSampleTime;
};
