#include "wifi101_adapter.h"
#include "CommandDistributor.h"

constexpr uint32_t SESSION_TIMEOUT_MS = 15000;

// The WiFi101 library closes a socket itself (freeing its receive buffer) when
// the peer disconnects, and may then hand the same socket number to a new
// connection. Only treat the socket as ours if it is still connected to the
// same remote port.
static bool ownsSocket(WiThrottleBuffers* wb)
{
  return wb->client.connected() && wb->client.remotePort() == wb->remotePort;
}

static void dropSession(WiThrottleSessions* w, size_t idx, const __FlashStringHelper* reason)
{
  WiThrottleBuffers* wb = &w->withrottle_buffers[idx];

  Serial.print(reason);
  Serial.print(F(" session "));
  Serial.print(idx);
  Serial.print(F(" port "));
  Serial.println(wb->remotePort);

  CommandDistributor::forget(idx);

  // Releases the ATWINC1500 socket and the library's malloc'd receive buffer.
  if(ownsSocket(wb))
  {
    wb->client.stop();
  }

  wb->client = WiFiClient();
  wb->inUse = false;
  wb->remotePort = 0;
  wb->inboundCnt = 0;
}

static void reapSessions(WiThrottleSessions* w)
{
  uint32_t now_ms = millis();

  for(size_t idx = 0; idx < WITHROTTLE_SESSION_MAX; idx++)
  {
    WiThrottleBuffers* wb = &w->withrottle_buffers[idx];

    if(!wb->inUse)
    {
      continue;
    }

    if(!ownsSocket(wb))
    {
      dropSession(w, idx, F("Disconnect"));
    }
    else if(now_ms - wb->last_active_ms >= SESSION_TIMEOUT_MS)
    {
      dropSession(w, idx, F("Timeout"));
    }
  }
}

static WiThrottleBuffers* findOrCreateSession(WiThrottleSessions* w, WiFiClient& client)
{
  uint16_t remote_port = client.remotePort();
  WiThrottleBuffers* free_wb = nullptr;
  size_t free_idx = 0;

  for(size_t idx = 0; idx < WITHROTTLE_SESSION_MAX; idx++)
  {
    WiThrottleBuffers* wb = &w->withrottle_buffers[idx];

    if(wb->inUse && wb->client == client && wb->remotePort == remote_port)
    {
      return wb;
    }

    if(!wb->inUse && free_wb == nullptr)
    {
      free_wb = wb;
      free_idx = idx;
    }
  }

  if(free_wb == nullptr)
  {
    return nullptr;
  }

  Serial.print(F("New client on port: "));
  Serial.print(remote_port);
  Serial.print(F(" session: "));
  Serial.println(free_idx);

  free_wb->inUse = true;
  free_wb->remotePort = remote_port;
  free_wb->inboundCnt = 0;

  // ensure ring buffer is clear
  while(free_wb->outboundRing.read() >= 0)
  {}

  free_wb->client = client;
  free_wb->last_active_ms = millis();
  return free_wb;
}

size_t openWiFiClients(WiThrottleSessions* w)
{
  size_t cnt = 0;

  for(size_t idx = 0; idx < WITHROTTLE_SESSION_MAX; idx++)
  {
    if(w->withrottle_buffers[idx].inUse)
    {
      cnt++;
    }
  }
  return cnt;
}

static void portParserOneLine(WiThrottleBuffers* wb, byte clientId, Stream& out)
{
  // Early exit on an empty line
  if(wb->inboundCnt == 0)
  {
    return;
  }

  // Null terminate and send along to the command distributor
  wb->inboundLine[wb->inboundCnt] = '\0';
  Serial.printf("cmd(%d): %s\n", clientId, wb->inboundLine);

  // remember start of outbound data
  wb->outboundRing.mark(clientId);
  CommandDistributor::parse(
    clientId,
    (byte*)wb->inboundLine,
    &wb->outboundRing);

  // Compute the length---inserted after the mark---or roll back.
  wb->outboundRing.commit();
  // Reset line to empty
  wb->inboundCnt = 0;

  // Now send response in outboundRing
  int ch = 0;
  Serial.printf("rsp(%d): ", clientId);

  if(wb->outboundRing.read() >= 0)
  {
    int count=wb->outboundRing.count();

    for(int j = 0; j < count; j++)
    {
      ch = wb->outboundRing.read();
      Serial.write(ch);
      out.write(ch);
    }
    wb->outboundRing.flush();
  }

  Serial.println(F("rsp--done"));

  // mark the time to keep the throttle and connection open
  wb->last_active_ms = millis();
}

void portParserLoop(WiFiServer *s, WiThrottleSessions* w)
{
  // Reap before available(): a socket the library closed and re-accepted for
  // a new peer is not "connected" until available() promotes it.
  reapSessions(w);

  // WiThrottle heartbeat: eStop locos of clients that have gone quiet.
  WiThrottle::loop(nullptr);

  // Return any client with input avaiable
  WiFiClient client = s->available();

  // client will be "false" if none of the clients have data
  if (!client)
  {
    // early exit if nothing to do
    return;
  }

  WiThrottleBuffers* wb = findOrCreateSession(w, client);

  if(wb == nullptr)
  {
    Serial.println(F("No session available, refusing client"));
    client.stop();
    return;
  }

  byte clientId = (byte)(wb - w->withrottle_buffers);

  int ch = client.read();

  // New-line marks the end of a withrottle statment
  if(ch == '\r' || ch == '\n')
  {
    portParserOneLine(wb, clientId, client);
  }
  else if(ch > 0)
  {
    if(wb->inboundCnt < INBOUND_LINE_MAX - 1)
    {
      wb->inboundLine[wb->inboundCnt++] = (char)ch;
    }
    else
    {
      Serial.println(F("Inbound line too long, discarding"));
      wb->inboundCnt = 0;
    }
  }
}
