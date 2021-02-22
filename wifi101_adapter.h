#ifndef _WIFI101_ADAPTER
#define _WIFI101_ADAPTER

#include <WiFi101.h>
#include "RingStream.h"

constexpr size_t INBOUND_LINE_MAX = 512;
typedef struct
{
    int32_t remotePort;
    char inboundLine[INBOUND_LINE_MAX];
    int32_t inboundCnt;
    RingStream outboundRing;
} WiThrottleBuffers;

constexpr size_t WITHROTTLE_SESSION_MAX = 8;
typedef struct
{
  WiThrottleBuffers withrottle_buffers[WITHROTTLE_SESSION_MAX];
} WiThrottleSessions;

typedef struct
{
  bool new_session;
  bool found;
  WiThrottleBuffers* wb;
} FindPortResult;

FindPortResult findRemotePort(WiThrottleSessions* w, int32_t remote_port);
void portParserOneLine(WiThrottleBuffers* wb, Stream& out);
void portParserLoop(WiFiServer *s, WiThrottleSessions* w);
void BeginWifi101Adapter(void);

#endif
