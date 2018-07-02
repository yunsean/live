#include <stdio.h>
#include "Rtmp2Publisher.h"
#include "xsystem.h"
#include "Writelog.h"
#include "log.h"

#define PACKET_BLOCK_COUNT	50
void RTMPLogCallback(int level, const char *fmt, va_list va) {
	char log[1024];
	vsnprintf(log, 1024, fmt, va);
	wli("%s", log);
}

const static char*	C_STATISTICS_METHOD("statistics");
const static char*	C_STATISTICS_PARAMETER("(Ljava/lang/Object;JJI)V");
CRtmp2Publisher::CRtmp2Publisher(const std::function<void(long sendBlocks, long lostBlocks, int bitrate)>& callback)
	: mMetadata()
	, mRtmpServer()

	, mPacketMutex()
	, mAllPackets()

	, mCallback(callback)

	, mRtmpClient(rtmpClean)
	, mStarted(false)
	, mRtmpThread()
	, mQuited(false)

	, mSentBlock(0)
	, mLostBlock(0) {
	RTMP_LogSetLevel(RTMP_LOGWARNING);
	RTMP_LogSetCallback(RTMPLogCallback);
}
CRtmp2Publisher::~CRtmp2Publisher() {
    cleanup();
}

void CRtmp2Publisher::setupVideo(const int width, const int height, const unsigned char* sps, const int szSps, const unsigned char* pps, const int szPps) {
	mMetadata.hasVideo = true;
	mMetadata.width = width;
	mMetadata.height = height;
	mMetadata.videoBitRate = width * height * 5;
	mMetadata.frameRate = 25;
	int sps_header_len(headerSize(sps, szSps));
	int pps_header_len(headerSize(pps, szPps));
	memcpy(mMetadata.sps, sps + sps_header_len, szSps - sps_header_len);
	mMetadata.spsLen = szSps - sps_header_len;
	memcpy(mMetadata.pps, pps + pps_header_len, szPps - pps_header_len);
	mMetadata.ppsLen = szPps - pps_header_len;
}
void CRtmp2Publisher::setupAudio(int channels, int sampleRate, const unsigned char* esds, const int szEsds) {
	mMetadata.hasAudio = true;
	mMetadata.channels = channels;
	mMetadata.sampleRate = sampleRate;
	mMetadata.audioBitRate = 64 * 1000;
	memcpy(mMetadata.esds, esds, szEsds);
	mMetadata.esdsLen = szEsds;
}

void CRtmp2Publisher::appendVideo(const unsigned char* datas, int length, long timecode, bool isKey, bool isNal /*= false*/, bool waitBlock) {
	if (!mStarted)return;
	if (isNal) return appendNal(datas, length, timecode, isKey, waitBlock);
	const unsigned char* begin(datas);
	const unsigned char* end(datas + length);
	const unsigned char* next(begin + 3);
	while (end - next > 3) {
		if (*(next + 0) == 0x00 && *(next + 1) == 0x00 && ((*(next + 2) == 0x00 && *(next + 3) == 0x01))) {
			appendNal(begin, next - begin, timecode, isKey, waitBlock);
			begin = next;			
			next = begin + 2;
		}
		next++;
	}
	if (end - begin > 3) {
		appendNal(begin, end - begin, timecode, isKey, waitBlock);
	}	
}
void CRtmp2Publisher::appendNal(const unsigned char* datas, int length, long timecode, bool isKey, bool waitBlock /*= false*/ ) {
	printf("%s %02x%02x%02x%02x%02x, %d\n", isKey ? " key" : "none" , datas[0], datas[1], datas[2], datas[3], datas[4], length);
	int header_len(headerSize(datas, length));
	int nalType(*(datas + header_len) & 0x1f);
	if (nalType > 5) return;
	datas += header_len;
	length -= header_len;
	RTMPPacket* lpPacket(DequeuePacket(0x05, RTMP_PACKET_TYPE_VIDEO, length + 5 + 4, timecode, waitBlock));
	if (lpPacket) {
		unsigned char* data((unsigned char*)lpPacket->m_body);
		data = write_8(data, isKey ? 0x17 : 0x27);
		data = write_8(data, 0x01);	//AVC NALU  
		data = write_24(data, 0x00);
		data = write_32(data, length);
		memcpy(data, datas, length);
		EnqueuePacket(lpPacket, false, false);
	} else {
		mLostBlock++;
		//wlw("drop a video frame.");
	}
}
void CRtmp2Publisher::appendAudio(const unsigned char* datas, int length, long timecode, bool waitBlock) {
	if (!mStarted)return;
	int header_len(headerSize(datas, length));
	datas += header_len;
	length -= header_len;
	RTMPPacket* lpPacket(DequeuePacket(0x04, RTMP_PACKET_TYPE_AUDIO, length + 2, timecode, waitBlock));
	if (lpPacket) {
		unsigned char* data((unsigned char*)lpPacket->m_body);
		data = write_8(data, 0xaf);
		data = write_8(data, 0x01);	//AAC RAW
		memcpy(data, datas, length);
		EnqueuePacket(lpPacket, false, true);
	} else {
		mLostBlock++;
		//wlw("drop a audio frame.");
	}
}
bool CRtmp2Publisher::connect(const char* url){
	disconnect();
	if (!mAllPackets.Initialize(PACKET_BLOCK_COUNT)) {
		return wlet(false, _T("Initialze packet cache failed."));
	}

    mQuited = false;
	mRtmpServer = url;
	CSmartHdr<RTMP*, void>	spRtmp(rtmpClean);
	wli("will connect");
	if (!doConnect(spRtmp)) {
		wle("connect to %s failed.", mRtmpServer.c_str());
		disconnect();
		return wlet(false, _T("connect rtmp server failed."));
	}
	wli("connected");
	mRtmpClient = spRtmp;
	mRtmpThread.run([this](){
		rtmpThread();
	});
	EnqueueRtmpHeader();
	mStarted = true;

	return true;
}
void CRtmp2Publisher::disconnect() {
	mStarted = false;
	mQuited = true;
	wli(_T("disconnect()"));
	mAllPackets.Finished();
	mRtmpThread.close();
	if (mRtmpClient) {
		mRtmpClient = NULL;
	}
	wli(_T("rtmp client released."));
}
void CRtmp2Publisher::cleanup() {
	disconnect();
}

unsigned char* CRtmp2Publisher::write_8(unsigned char* buffer, unsigned int v) {
	buffer[0] = (uint8_t)v;
	return buffer + 1;
}
unsigned char* CRtmp2Publisher::write_16(unsigned char* buffer, unsigned int v) {
	buffer[0] = (uint8_t)(v >> 8);
	buffer[1] = (uint8_t)(v >> 0);
	return buffer + 2;
}
unsigned char* CRtmp2Publisher::write_24(unsigned char* buffer, unsigned int v) {
	buffer[0] = (uint8_t)(v >> 16);
	buffer[1] = (uint8_t)(v >> 8);
	buffer[2] = (uint8_t)(v >> 0);
	return buffer + 3;
}
unsigned char* CRtmp2Publisher::write_32(unsigned char* buffer, uint32_t v) {
	buffer[0] = (uint8_t)(v >> 24);
	buffer[1] = (uint8_t)(v >> 16);
	buffer[2] = (uint8_t)(v >> 8);
	buffer[3] = (uint8_t)(v >> 0);
	return buffer + 4;
}

void CRtmp2Publisher::EnqueueRtmpHeader() {
	CByte metadata;
	CMetadata::onMetadata(&mMetadata, metadata);
	RTMPPacket* lpPacket(NULL);
	lpPacket = DequeuePacket(0x03, RTMP_PACKET_TYPE_INFO, metadata.GetSize(), 0);
	wli(_T("will send header: lpPacket=%p, video:%d, audio:%d"), lpPacket, mMetadata.hasVideo, mMetadata.hasAudio);
	if (lpPacket != NULL) {
		memcpy(lpPacket->m_body, metadata.GetData(), metadata.GetSize());
	    wli(_T("send metadata, %d bytes"), metadata.GetSize());
		EnqueuePacket(lpPacket, true, false);
	}
	if (mMetadata.hasVideo) {
		EnqueueVideoConfigPacket();
	}
	if (mMetadata.hasAudio) {
		EnqueueAudioConfigPacket();
	}
}
void CRtmp2Publisher::EnqueueAudioConfigPacket() {
#define FLV_AAC_SEQUENCE_HEADER     (0)

	if (mMetadata.esdsLen < 1)return;
	int size = 1 + 1 + mMetadata.esdsLen;
	RTMPPacket* lpPacket(DequeuePacket(0x03, RTMP_PACKET_TYPE_AUDIO, size, 0));
	if (lpPacket == NULL)return;
	unsigned char* data = (unsigned char*)lpPacket->m_body;

	data = write_8(data, 0xaf);
	data = write_8(data, FLV_AAC_SEQUENCE_HEADER);
	memcpy(data, mMetadata.esds, mMetadata.esdsLen);
	wli(_T("send audio config, %d bytes"), mMetadata.esdsLen);
	data += mMetadata.esdsLen;
	EnqueuePacket(lpPacket, true, false);
}
void CRtmp2Publisher::EnqueueVideoConfigPacket() {
#define FLV_AVC_SEQUENCE_HEADER     (0)

	if (mMetadata.spsLen < 7)return;
	/*FrameType&FrameType[1] + AVCPacketType[1] + CompositionTime[3] + 1[1] + profileIndication[1]
	+ profileCompatibility[1] + levelIndication[1] + 0xFF[1] + 0xE1[1] + sps_size[2] + sps
	+ pps_count[1] + pps_len[2] + pps*/
	int size = 1 + 1 + 3 + 1 + 1
		+ 1 + 1 + 1 + 1 + 2 + mMetadata.spsLen
		+ 1 + 2 + mMetadata.ppsLen; 
	RTMPPacket* lpPacket(DequeuePacket(0x03, RTMP_PACKET_TYPE_VIDEO, size, 0));
	if (lpPacket == NULL)return;
	unsigned char* data = (unsigned char*)lpPacket->m_body;

	int profileIndication(mMetadata.sps[1]);
	int profileCompatibility(mMetadata.sps[2]);
	int levelIndication(mMetadata.sps[3]);
	data = write_8(data, 0x17);
	data = write_8(data, FLV_AVC_SEQUENCE_HEADER);
	data = write_24(data, 0);
	data = write_8(data, 1);
	data = write_8(data, profileIndication);
	data = write_8(data, profileCompatibility);
	data = write_8(data, levelIndication);

	data = write_8(data, 0xFF);
	data = write_8(data, 0xE1);
	data = write_16(data, mMetadata.spsLen);
	memcpy(data, mMetadata.sps, mMetadata.spsLen);
	data += mMetadata.spsLen;

	data = write_8(data, 1);
	data = write_16(data, mMetadata.ppsLen);
	memcpy(data, mMetadata.pps, mMetadata.ppsLen);
	data += mMetadata.ppsLen;
	wli(_T("send video config, %d bytes"), mMetadata.spsLen + mMetadata.ppsLen);
	EnqueuePacket(lpPacket, true, false);
}
RTMPPacket* CRtmp2Publisher::DequeuePacket(int channel, unsigned int packetType, unsigned int size, unsigned int timestamp, bool waitBlock) {
	CRTMPPacket* packet(NULL);
	std::lock_guard<std::recursive_mutex> lock(mPacketMutex);
	if (!mAllPackets.GetEmptyBlock(packet, waitBlock ? INFINITE : 10))return NULL;
	RTMPPacket_Reset(packet);
	RTMPPacket_Free(packet);
	RTMPPacket_Alloc(packet, size);
	packet->m_packetType = packetType;
	packet->m_nChannel = channel;
	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet->m_hasAbsTimestamp = 1;
	packet->m_nTimeStamp = timestamp;
	packet->m_nBodySize = size;
	return packet;
}
void CRtmp2Publisher::EnqueuePacket(RTMPPacket* packet, bool isDirect, bool isAudio) {
    mAllPackets.ReturnValidBlock((CRTMPPacket*)packet);
}

void CRtmp2Publisher::rtmpClean(RTMP* rtmp) {
	RTMP_Close(rtmp);
	RTMP_Free(rtmp);
}
bool CRtmp2Publisher::doConnect(CSmartHdr<RTMP*, void>& shRtmp) {
	mAllPackets.Reset();
	shRtmp = RTMP_Alloc();
	if (shRtmp == NULL)return wlet(false, _T("RTMP_Alloc() == NULL"));
	RTMP_Init(shRtmp);
	shRtmp->Link.timeout = 3;
	bool result(RTMP_SetupURL(shRtmp, (char*)mRtmpServer.c_str()) != 0);
	if (!result)return wlet(false, _T("RTMP_SetupURL(0x%p, %s) failed."), shRtmp.GetHdr(), mRtmpServer.c_str());
	RTMP_EnableWrite(shRtmp);
	char szLive[10] = "live";
	char szTrue[10] = "true";
	AVal avLive = { szLive, 4 };
	AVal avTrue = { szTrue, 4 };
	result = RTMP_SetOpt(shRtmp, &avLive, &avTrue) != 0;
	if (!result)return wlet(false, "RTMP_SetOpt(live, true) == %d", result);
	result = RTMP_Connect(shRtmp, NULL) != 0;
	if (!result)return wlet(false, _T("RTMP_Connect() failed."));
	result = RTMP_ConnectStream(shRtmp, 0) != 0;
	if (!result)return wlet(false, _T("RTMP_ConnectStream() failed."));

	return true;
}
int CRtmp2Publisher::headerSize(const unsigned char* data, const int size) {
	if (size < 3)return 0;
	int header_len(0);
	if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01) header_len = 3;
	else if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01) header_len = 4;
	return header_len;
}
void CRtmp2Publisher::rtmpThread() {
	wli(_T("rtmpThread(%d) in"), std::tid());
	unsigned int lastTick(std::tickCount());
	unsigned int sentBlock(0);
	unsigned int lostBlock(0);
	int sentBytes(0);
	while (!mQuited && !mAllPackets.IsFinished()) {
		if (mRtmpClient == NULL) {
			CSmartHdr<RTMP*, void> spRtmp(rtmpClean);
			wlw(_T("Try reconnect to rtmp server:%s"), mRtmpServer.c_str());
			if (!doConnect(spRtmp)) {
                for (int i = 0; i < 10 && !mAllPackets.IsFinished(); i++) {
				    std::sleep(500);
				}
				continue;
			}
			wlw(_T("Reconnect to rtmp server:%s ok."), mRtmpServer.c_str());
			std::lock_guard<std::recursive_mutex> lock(mPacketMutex);
            mAllPackets.Reset();
			EnqueueRtmpHeader();
			mRtmpClient = spRtmp;
		}
		while (!mQuited && mRtmpClient && RTMP_IsConnected(mRtmpClient)) {
			CRTMPPacket* lpPacket(NULL);
			if (!mAllPackets.GetValidBlock(lpPacket, INFINITE))break;
			lpPacket->m_nInfoField2 = mRtmpClient->m_stream_id;
			if (!RTMP_SendPacket(mRtmpClient, lpPacket, 0)) {
				mLostBlock++;
				wlw(_T("send rtmp packet failed."));
			} else {
				mSentBlock++;
				sentBytes += lpPacket->m_nBodySize;
			}
			RTMPPacket_Free(lpPacket);
			mAllPackets.ReturnEmptyBlock(lpPacket);
			if (std::tickCount() - lastTick >= 2000) {
				int bitrate = sentBytes * 8 / (std::tickCount() - lastTick);
				if (mCallback != nullptr) mCallback(mSentBlock, mLostBlock, bitrate);
				lastTick = std::tickCount();
				sentBytes = 0;
			}
		}
		wlw(_T("rtmp disconnected."));
		mRtmpClient = NULL;
	}
	wli(_T("rtmpThread(%d) out"), std::tid());
}

