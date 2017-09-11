#pragma once
#include "RtmpMessage.h"
//////////////////////////////////////////////////////////////////////////
// amf0 command message, command name macros
#define RTMP_AMF0_COMMAND_CONNECT               "connect"
#define RTMP_AMF0_COMMAND_CREATE_STREAM         "createStream"
#define RTMP_AMF0_COMMAND_CLOSE_STREAM          "closeStream"
#define RTMP_AMF0_COMMAND_PLAY                  "play"
#define RTMP_AMF0_COMMAND_PAUSE                 "pause"
#define RTMP_AMF0_COMMAND_ON_BW_DONE            "onBWDone"
#define RTMP_AMF0_COMMAND_ON_STATUS             "onStatus"
#define RTMP_AMF0_COMMAND_RESULT                "_result"
#define RTMP_AMF0_COMMAND_ERROR                 "_error"
#define RTMP_AMF0_COMMAND_RELEASE_STREAM        "releaseStream"
#define RTMP_AMF0_COMMAND_DELETE_STREAM			"deleteStream"
#define RTMP_AMF0_COMMAND_FC_PUBLISH            "FCPublish"
#define RTMP_AMF0_COMMAND_FC_UNPUBLISH          "FCUnpublish"
#define RTMP_AMF0_COMMAND_PUBLISH               "publish"
#define RTMP_AMF0_COMMAND_GET_STREAMLENGTH		"getStreamLength"
#define RTMP_AMF0_DATA_SAMPLE_ACCESS            "|RtmpSampleAccess"

template<int CSID, int TYPE>
class CCommandMessage : public CAmfxMessage<CSID, RTMP_MSG_AMF0CommandMessage, TYPE> {
public:
	virtual ~CCommandMessage() {}
};

//////////////////////////////////////////////////////////////////////////
//Connect App
class CConnectAppRequest : public CCommandMessage<RTMP_CID_OverConnection, ConnectAppRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Object> command;
	CSmartPtr<amf0::Amf0Object> args;
public:
	CConnectAppRequest() : name(RTMP_AMF0_COMMAND_CONNECT), transactionId(0), command(nullptr), args(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)command.GetPtr(), (amf0::Amf0Any*&)args.GetPtr());
};
class CConnectAppResponse : public CCommandMessage<RTMP_CID_OverConnection, ConnectAppResponse> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Object> props;
	CSmartPtr<amf0::Amf0Object> info;
public:
	CConnectAppResponse(double tid) : name(RTMP_AMF0_COMMAND_RESULT), transactionId(tid), props(nullptr), info(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), (amf0::Amf0Any*&)info.GetPtr());
};

//////////////////////////////////////////////////////////////////////////
//Release Stream
class CReleaseStreamRequest : public CCommandMessage<RTMP_CID_OverStream, ReleaseStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
public:
	CReleaseStreamRequest() : name(RTMP_AMF0_COMMAND_RELEASE_STREAM), transactionId(1), props(nullptr), streamName() {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName);
};

//////////////////////////////////////////////////////////////////////////
//FC Publish Stream
class CFCPublishStreamRequest : public CCommandMessage<RTMP_CID_OverConnection, FCPublishStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
public:
	CFCPublishStreamRequest() : name(RTMP_AMF0_COMMAND_FC_PUBLISH), transactionId(1), props(nullptr), streamName() {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName);
};

//////////////////////////////////////////////////////////////////////////
//FC Publish Stream
class CFCUnpublishStreamRequest : public CCommandMessage<RTMP_CID_OverConnection, FCUnpublishStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
public:
	CFCUnpublishStreamRequest() : name(RTMP_AMF0_COMMAND_FC_UNPUBLISH), transactionId(1), props(nullptr), streamName() {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName);
};
class CUnpublishStreamResponse : public CCommandMessage<RTMP_CID_OverStream, FCUnpublishStreamResponse> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Any> props;
	CSmartPtr<amf0::Amf0Object> info;
public:
	CUnpublishStreamResponse(double tid) : name(RTMP_AMF0_COMMAND_ON_STATUS), transactionId(tid), props(nullptr), info(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), (amf0::Amf0Any*&)info.GetPtr());
};

//////////////////////////////////////////////////////////////////////////
//Create Stream
class CCreateStreamRequest : public CCommandMessage<RTMP_CID_OverConnection, CreateStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
public:
	CCreateStreamRequest() : name(RTMP_AMF0_COMMAND_CREATE_STREAM), transactionId(1), props(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr());
};
class CCreateStreamResponse : public CCommandMessage<RTMP_CID_OverConnection, CreateStreamResponse> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Any> props;
	double streamId;
public:
	CCreateStreamResponse(double tid) : name(RTMP_AMF0_COMMAND_RESULT), transactionId(tid), props(nullptr), streamId(0) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamId);
};

//////////////////////////////////////////////////////////////////////////
//Delete Stream
class CDeleteStreamRequest : public CCommandMessage<RTMP_CID_OverStream, DeleteStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	double streamId;
public:
	CDeleteStreamRequest() : name(RTMP_AMF0_COMMAND_DELETE_STREAM), transactionId(1), props(nullptr), streamId(0) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamId);
};

//////////////////////////////////////////////////////////////////////////
//Close Stream
class CCloseStreamRequest : public CCommandMessage<RTMP_CID_OverStream, CloseStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
public:
	CCloseStreamRequest() : name(RTMP_AMF0_COMMAND_CLOSE_STREAM), transactionId(1), props(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr());
};

//////////////////////////////////////////////////////////////////////////
//Publish Stream
class CPublishStreamRequest : public CCommandMessage<RTMP_CID_OverConnection2, PublishStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
	std::string type;
public:
	CPublishStreamRequest() : name(RTMP_AMF0_COMMAND_PUBLISH), transactionId(1), props(nullptr), streamName(), type() {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName, type);
};
class CPublishStreamResponse : public CCommandMessage<RTMP_CID_OverStream, PublishStreamResponse> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Any> props;
	CSmartPtr<amf0::Amf0Object> info;
public:
	CPublishStreamResponse(double tid) : name(RTMP_AMF0_COMMAND_ON_STATUS), transactionId(tid), props(nullptr), info(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), (amf0::Amf0Any*&)info.GetPtr());
};

//////////////////////////////////////////////////////////////////////////
//Play Stream
class CPlayStreamRequest : public CCommandMessage<RTMP_CID_OverConnection2, PlayStreamRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
	double start;
	double duration;
	bool reset;
public:
	CPlayStreamRequest() : name(RTMP_AMF0_COMMAND_PLAY), transactionId(1), props(nullptr), streamName(), start(-1), duration(-1), reset(false) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName, start, duration, reset);
};
class CPlayStreamResponse : public CCommandMessage<RTMP_CID_OverStream, PlayStreamResponse> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Any> props;
	CSmartPtr<amf0::Amf0Object> desc;
public:
	CPlayStreamResponse(double tid) : name(RTMP_AMF0_COMMAND_ON_STATUS), transactionId(tid), props(nullptr), desc(nullptr) {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), (amf0::Amf0Any*&)desc.GetPtr());
};

//////////////////////////////////////////////////////////////////////////
//Get Stream Length
class CGetStreamLengthRequest : public CCommandMessage<RTMP_CID_OverStream2, GetStreamLengthRequest> {
public:
	std::string name;
	double transactionId;
	CSmartPtr<amf0::Amf0Null> props;
	std::string streamName;
public:
	CGetStreamLengthRequest() : name(RTMP_AMF0_COMMAND_PLAY), transactionId(1), props(nullptr), streamName() {}
	MESSAGEPACKET_METHOD(name, transactionId, (amf0::Amf0Any*&)props.GetPtr(), streamName);
};

