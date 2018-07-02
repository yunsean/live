#pragma once
#include <string>
#include <stdio.h>
#include <vector>
#include <queue>
#include "Metadata.h"
#include "SmartPtr.h"
#include "SmartHdr.h"
#include "SmartBlk.h"
#include "rtmp.h"
#include "xthread.h"
#include "xstring.h"

class CRtmp2Publisher {
public:
	CRtmp2Publisher(const std::function<void(long sendBlocks, long lostBlocks, int bitrate)>& callback = nullptr);
	~CRtmp2Publisher();

public:
    void setupVideo(const int width, const int height, const unsigned char* sps, const int szSps, const unsigned char* pps, const int szPps);
	void setupAudio(int channels, int sampleRate, const unsigned char* esds, const int szEsds);
    void appendVideo(const unsigned char* datas, int length, long timecode, bool isKey, bool isNal = false, bool waitBlock = false);
	void appendAudio(const unsigned char* datas, int length, long timecode, bool waitBlock = false);
    bool connect(const char* url);
    void disconnect();
    void cleanup();

public:
	unsigned char*  write_8(unsigned char* buffer, unsigned int v);
	unsigned char*  write_16(unsigned char* buffer, unsigned int v);
	unsigned char*  write_24(unsigned char* buffer, unsigned int v);
	unsigned char*  write_32(unsigned char* buffer, uint32_t v);

protected:
	void appendNal(const unsigned char* datas, int length, long timecode, bool isKey, bool waitBlock = false);
	RTMPPacket* DequeuePacket(int channel, unsigned int packetType, unsigned int size, unsigned int timestamp, bool waitBlock = false);
	void EnqueuePacket(RTMPPacket* packet, bool isDirect, bool isAudio);
	void EnqueueVideoConfigPacket();
	void EnqueueAudioConfigPacket();
	void EnqueueRtmpHeader();

	bool doConnect(CSmartHdr<RTMP*, void>& shRtmp);
	void rtmpThread();
	int headerSize(const unsigned char* data, const int size);

protected:
	static void	rtmpClean(RTMP* rtmp);

protected:
    enum {mp3 = 0, aac};

protected:
	class CRTMPPacket : public RTMPPacket {
	public:
		CRTMPPacket() {
			memset(this, 0, sizeof(RTMPPacket));
		}
	};
	
private:
	RTMPMetadata mMetadata;
	xstring<char> mRtmpServer;
	std::function<void(long sendBlocks, long lostBlocks, int bitrate)> mCallback;

	std::recursive_mutex mPacketMutex;
	CSmartBlk<CRTMPPacket> mAllPackets;

	CSmartHdr<RTMP*, void> mRtmpClient;
	bool mStarted;
	xthread mRtmpThread;
	bool mQuited;

	unsigned int mSentBlock;
	unsigned int mLostBlock;
};