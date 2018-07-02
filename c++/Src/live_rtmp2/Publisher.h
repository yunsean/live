#pragma once
#include <functional>

#ifdef _WIN32
#ifdef LIVE_RTMP2_EXPORTS
#define RTMP2_API __declspec(dllexport)
#else
#define RTMP2_API __declspec(dllimport)
#endif
#else 
#define RTMP2_API
#endif

class CRtmp2Publisher;
class RTMP2_API CPublisher {
public:
	CPublisher(const std::function<void(long sendBlocks, long lostBlocks, int bitrate)>& callback = nullptr);
	~CPublisher();

public:
	void setupVideo(const int width, const int height, const unsigned char* sps, const int szSps, const unsigned char* pps, const int szPps);
	void setupAudio(int channels, int sampleRate, const unsigned char* esds, const int szEsds);
	bool connect(const char* url);
	void appendVideo(const unsigned char* datas, int length, long timecode, bool isKey, bool isNal = false, bool waitBlock = false);
	void appendAudio(const unsigned char* datas, int length, long timecode, bool waitBlock = false);
	void disconnect();
	void cleanup();

private:
	CRtmp2Publisher* m_publisher;
};

