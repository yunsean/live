#pragma once
#include <vector>
#include "SourceInterface.h"
#include "xstring.h"

class CForwardProxy : public ISourceProxy {
public:
	CForwardProxy(const xtstring& device, const xtstring& moniker, const xtstring& param, const xtstring& beginTime = _T(""), const xtstring& endTime = _T(""));
	CForwardProxy(const xtstring& device, const xtstring& moniker, const xtstring& param, std::map<xtstring, xtstring> extend);
	~CForwardProxy(void);

public:
	int openSuperior(const std::vector<xtstring>& superiors, const int premier);
	bool openSuperior(const xtstring& superior);

protected:
	virtual LPCTSTR			SourceName() const;
	virtual unsigned int    MaxStartDelay(const unsigned int def) { return def; }
	virtual unsigned int    MaxFrameDelay(const unsigned int def) { return def; }
	virtual bool			StartFetch(ISourceProxyCallback* callback);
	virtual void			WantKeyFrame();
	virtual void			StopFetch();
	virtual bool			PTZControl(const PTZAction action, const int value);
	virtual bool			VideoEffect(const int bright, const int contrast, const int saturation, const int hue);
	virtual bool			Destroy();

private:
	std::string buildQuery(const std::map<xtstring, xtstring>& query);
	std::string uriEncode(const xtstring& uri);

private:
	xtstring m_strDevice;
	xtstring m_strMoniker;
	xtstring m_strParam;
	xtstring m_strPath;
	std::string m_strHost;
	bool m_vod;
};

