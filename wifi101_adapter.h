#ifndef _WIFI101_ADAPTER
#define _WIFI101_ADAPTER

#include <WiFi101.h>
#include "RingStream.h"
#include "WiThrottle.h"

constexpr size_t INBOUND_LINE_MAX = 512;
typedef struct
{
    bool inUse;
    uint16_t remotePort;
    char inboundLine[INBOUND_LINE_MAX];
    size_t inboundCnt;
    RingStream outboundRing;

    WiFiClient client;
    unsigned long last_active_ms;
} WiThrottleBuffers;

// The session index (not the remote port) is the clientId handed to
// CommandDistributor/WiThrottle, so it must fit in a byte.
constexpr size_t WITHROTTLE_SESSION_MAX = 8;
typedef struct
{
  WiThrottleBuffers withrottle_buffers[WITHROTTLE_SESSION_MAX];
} WiThrottleSessions;

void portParserLoop(WiFiServer *s, WiThrottleSessions* w);
size_t openWiFiClients(WiThrottleSessions* w);

#endif
