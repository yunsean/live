#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "CodecInterface.h"
#include "NalDemuxer.h"
#include "SmartHdr.h"
#include "SmartPtr.h"

class CNalFactory : public IDecodeFactory {
public:
	CNalFactory();
	~CNalFactory();

public:
	static CNalFactory& singleton();

protected:
	virtual LPCTSTR			FactoryName() const;
	virtual bool			Initialize();
	virtual bool			DidSupport(const uint8_t* const header, const int length);
	virtual IDecodeProxy*	CreateDecoder(IDecodeProxyCallback* callback, const uint8_t* const header, const int length);
	virtual bool			GetStatusInfo(LPSTR* json, void(**free)(LPSTR));
	virtual void			Uninitialize();
	virtual void			Destroy();

private:
	const LPCTSTR m_lpszFactoryName;
};

