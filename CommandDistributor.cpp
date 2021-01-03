/*
 *  © 2020,Gregor Baues,  Chris Harlow. All rights reserved.
 *
 *  This file is part of CommandStation-EX
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with CommandStation.  If not, see <https://www.gnu.org/licenses/>.
 */
#include <Arduino.h>
#include "CommandDistributor.h"
#include "WiThrottle.h"

// Initialize DCC++ parser to null
DCCEXParser* CommandDistributor::parser = nullptr;

void  CommandDistributor::parse(byte clientId,byte * buffer, RingStream * streamer)
{
  // Auto-detect DCC++ commands
  if (buffer[0] == '<')
  {
    // If there parser has not been created, allocate once
    if (!parser)
    {
      parser = new DCCEXParser();
    }
    parser->parse(streamer, buffer, true); // tell JMRI parser that ACKS are blocking because we can't handle the async
  }
  else
  {
    // WiThrottle protocol commands start with a character
    WiThrottle::getThrottle(clientId)->parse(streamer, buffer);
  }
}
