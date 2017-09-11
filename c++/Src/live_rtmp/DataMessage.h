#pragma once
#include "RtmpMessage.h"

#define RTMP_AMF0_DATA_SET_DATAFRAME	"@setDataFrame"
#define RTMP_AMF0_DATA_ON_METADATA		"onMetaData"

template<int TYPE>
class CDataMessage : public CAmfxMessage<RTMP_CID_OverConnection, RTMP_MSG_AMF0DataMessage, TYPE> {
public:
	virtual ~CDataMessage() {}
};

//////////////////////////////////////////////////////////////////////////
//Release Stream
class COnMetadataRequest : public CDataMessage<OnMetadataRequest> {
public:
	std::string command;
	std::string name;
	CSmartPtr<amf0::Amf0Any> metadata;
public:
	COnMetadataRequest() : command(RTMP_AMF0_DATA_SET_DATAFRAME), name(RTMP_AMF0_DATA_ON_METADATA), metadata(nullptr) {}
	MESSAGEPACKET_METHOD(command, name, (amf0::Amf0Any*&)metadata.GetPtr());
};

