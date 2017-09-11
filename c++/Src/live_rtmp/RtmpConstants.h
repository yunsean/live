#pragma once

//////////////////////////////////////////////////////////////////////////
//Chunk Stream ID
#define RTMP_CID_ProtocolControl                0x02
#define RTMP_CID_OverConnection                 0x03
#define RTMP_CID_OverConnection2                0x04
#define RTMP_CID_OverStream                     0x05
#define RTMP_CID_OverStream2                    0x08
#define RTMP_CID_Video                          0x06
#define RTMP_CID_Audio                          0x07

//////////////////////////////////////////////////////////////////////////
//Message Type
#define RTMP_MSG_AudioMessage                   8 // 0x08
#define RTMP_MSG_VideoMessage                   9 // 0x09

#define RTMP_MSG_AMF3CommandMessage             17 // 0x11
#define RTMP_MSG_AMF0CommandMessage             20 // 0x14

#define RTMP_MSG_AMF0DataMessage                18 // 0x12
#define RTMP_MSG_AMF3DataMessage                15 // 0x0F

#define RTMP_MSG_AMF3SharedObject               16 // 0x10
#define RTMP_MSG_AMF0SharedObject               19 // 0x13

#define RTMP_MSG_AggregateMessage               22 // 0x16

//////////////////////////////////////////////////////////////////////////
//
#define RTMP_SIG_FMS_VER                        "3,5,3,888"
#define RTMP_SIG_AMF0_VER                       0
#define RTMP_SIG_CLIENT_ID                      "ASAICiss"

//////////////////////////////////////////////////////////////////////////
//onStatus
#define StatusLevel                             "level"
#define StatusCode                              "code"
#define StatusDescription                       "description"
#define StatusDetails                           "details"
#define StatusClientId                          "clientid"
#define StatusLevelStatus                       "status"
#define StatusLevelError                        "error"
#define StatusCodeConnectSuccess                "NetConnection.Connect.Success"
#define StatusCodeConnectRejected               "NetConnection.Connect.Rejected"
#define StatusCodeStreamReset                   "NetStream.Play.Reset"
#define StatusCodeStreamStart                   "NetStream.Play.Start"
#define StatusCodeStreamPause                   "NetStream.Pause.Notify"
#define StatusCodeStreamUnpause                 "NetStream.Unpause.Notify"
#define StatusCodePublishStart                  "NetStream.Publish.Start"
#define StatusCodeDataStart                     "NetStream.Data.Start"
#define StatusCodeUnpublishSuccess              "NetStream.Unpublish.Success"
