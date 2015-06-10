#ifndef CHANNEL_HISTORY_H
#define CHANNEL_HISTORY_H


#include <deque>
#include "ChannelList.h"
#include "Settings.h"


class CChannelHistory
{
public:
	CChannelHistory();
	~CChannelHistory();
	void Clear();
	bool SetCurrentChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	const CTunerChannelInfo *Forward();
	const CTunerChannelInfo *Backward();

private:
	std::deque<CTunerChannelInfo*> m_ChannelList;
	int m_MaxChannelHistory;
	int m_CurrentChannel;
};

class CRecentChannelList : public CSettingsBase
{
public:
	CRecentChannelList();
	~CRecentChannelList();
	int NumChannels() const;
	void Clear();
	const CTunerChannelInfo *GetChannelInfo(int Index) const;
	bool Add(const CTunerChannelInfo &ChannelInfo);
	bool Add(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	bool SetMenu(HMENU hmenu,bool fClear=true) const;
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;

private:
	std::deque<CTunerChannelInfo*> m_ChannelList;
	int m_MaxChannelHistory;
	int m_MaxChannelHistoryMenu;
};


#endif
