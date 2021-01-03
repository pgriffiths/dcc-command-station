#include "wifi101_adapter.h"
#include "CommandDistributor.h"

FindPortResult findRemotePort(WiThrottleSessions* w, int32_t remote_port)
{
  // Is this a client we have seen before?
  bool new_session = true;
  bool found = false;
  size_t idx;

  // First look for remote_port
  for(idx = 0; idx < WITHROTTLE_SESSION_MAX; idx++)
  {
    if(w->withrottle_buffers[idx].remotePort == remote_port)
    {
      new_session = false;
      break;
    }
  }

  // If this is a new_session, look for first available buffers (marked with remotePort==0)
  if(new_session)
  {
    for(idx = 0; idx < WITHROTTLE_SESSION_MAX; idx++)
    {
      if(w->withrottle_buffers[idx].remotePort == 0)
      {
        found = true;
        break; // found!
      }
    }
  }

  // Setup new session
  if(new_session && found)
  {
    // New client!
    WiThrottleBuffers* wb = &w->withrottle_buffers[idx];

    // Create new parser, insert into map and get iterator to new element
    Serial.print(F("New client on port: "));
    Serial.println(remote_port);

    // session is in use now!
    wb->remotePort = remote_port;
    wb->inboundCnt = 0;
  }

  // whether new or not, the session is at index idx.
  return (FindPortResult) {
    .new_session = new_session,
    .found = (idx != WITHROTTLE_SESSION_MAX),
    .wb = &w->withrottle_buffers[idx]
  };
}

void portParserOneLine(WiThrottleBuffers* wb, Stream& out)
{
  // Early exit on an empty line
  if(wb->inboundCnt == 0)
  {
    return;
  }

  // Null terminate and send along to the command distrubtor
  wb->inboundLine[wb->inboundCnt++] = '\0';
  Serial.println(F("cmd:"));
  Serial.print(wb->inboundLine);
  Serial.println();

  // note that the remote_port number is the "clientId"
  // remember start of outbound data
  wb->outboundRing.mark(wb->remotePort);
  CommandDistributor::parse(
    wb->remotePort,
    (byte*)wb->inboundLine,
    &wb->outboundRing);
  // Compute the length---inserted after the mark---or roll back.
  wb->outboundRing.commit();
  // Reset line to empty
  wb->inboundCnt = 0;

  // Now send response in outboundRing
  int ch = 0;
  Serial.println(F("Rsp:"));

  if(wb->outboundRing.read() >= 0)
  {
    int count=wb->outboundRing.count();

    for(size_t j = 0; j < count; j++)
    {
      ch = wb->outboundRing.read();
      Serial.write(ch);
      out.write(ch);
    }
    wb->outboundRing.flush();
  }
}

void portParserLoop(WiFiServer *s, WiThrottleSessions* w)
{
  // Return any client with input avaiable
  WiFiClient client = s->available();

  // client will be "false" if none of the clients have data
  if (!client)
  {
    // early exit if nothing to do
    return;
  }

  // Have a client with data on a remote port
  int32_t remote_port = (int32_t)client.remotePort();
  // Determine if
  auto it = findRemotePort(w, remote_port);

  if(!it.found)
  {
    Serial.println(F("No session available"));
    client.stop();
    return;
  }

  WiThrottleBuffers* wb = it.wb;

  // Read data if connected -- else close
  if(client.connected())
  {
    // Read until a line break
    int ch = client.read();

    // New-line marks the end of a withrottle statment
    if(ch == '\r' || ch == '\n')
    {
      portParserOneLine(wb, client);
    }
    else // Not a new line, just add to input
    {
      if(ch > 0)
      {
        wb->inboundLine[wb->inboundCnt++] = (char)ch;
        // handle over-flow of line buffer!!!
      }
    }
  }
  else
  {
    // close the connection:
    Serial.println(F("disconnect"));
    client.stop();
    wb->remotePort = 0;
  }
}
