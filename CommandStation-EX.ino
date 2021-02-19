////////////////////////////////////////////////////////////////////////////////////
//  © 2020, Chris Harlow. All rights reserved.
//
//  This file is a demonstattion of setting up a DCC-EX
// Command station with optional support for direct connection of WiThrottle devices
// such as "Engine Driver". If you contriol your layout through JMRI
// then DON'T connect throttles to this wifi, connect them to JMRI.
//
//  THE WIFI FEATURE IS NOT SUPPORTED ON ARDUINO DEVICES WITH ONLY 2KB RAM.
////////////////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include <SPI.h>
#include <WiFi101.h>
#include <periodic_trigger.h>
#include <smooth_on_off.h>

#include "config.h"
#include "DCCEX.h"
#include "wifi101_adapter.h"
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// the IP address for the shield:
// IPAddress dcc_ip(192, 168, 0, 12);
WiFiServer *server;

WiThrottleSessions withrottle_sessions;

// Create a serial command parser for the USB connection,
// This supports JMRI or manual diagnostics and commands
// to be issued from the USB serial console.
DCCEXParser serialParser;

constexpr int CATSEYE_PRESS_PIN = 13;
constexpr int CATSEYE_BLUE_PIN = 5;
constexpr int CATSEYE_RED_PIN = 6;
constexpr int FLASH_MS = 1200;
constexpr int TRANSITION_MS = 400;

SmoothOnOff catseye_red(CATSEYE_BLUE_PIN, TRANSITION_MS);
SmoothOnOff catseye_blue(CATSEYE_RED_PIN, TRANSITION_MS);
PeriodicTrigger quick_timer(FLASH_MS);
PeriodicTrigger slow_timer(5*FLASH_MS);

void printWiFiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print(F("SSID: "));
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(F("IP Address: "));
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print(F("signal strength (RSSI):"));
  Serial.print(rssi);
  Serial.println(F(" dBm"));
}

void setupWifi()
{
#if defined(ARDUINO_SAMD_ZERO)
  // Configure pins for Adafruit ATWINC1500 Feather
  WiFi.setPins(8,7,4,2);
#endif

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println(F("WiFi shield not present"));
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  uint8_t status = WL_IDLE_STATUS;
  while ( status != WL_CONNECTED)
  {
    // Serial.print(F("Attempting to connect to SSID: "));
    // Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    if(status == WL_CONNECTED)
    {
      // connected
      break;
    }
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWiFiStatus();
}

void printFreeMem()
{
  Serial.print(F("Free RAM (bytes)="));
  Serial.println(freeMemory());
}

MotorDriver main_track = MotorDriver(16, 11, UNUSED_PIN, UNUSED_PIN, A0, 2.99, 2000, UNUSED_PIN);
MotorDriver prog_track = MotorDriver(17, 12, UNUSED_PIN, UNUSED_PIN, A1, 2.99, 2000, UNUSED_PIN);

void setup()
{
  for(size_t idx=0; idx < WITHROTTLE_SESSION_MAX; idx++)
  {
    withrottle_sessions.withrottle_buffers[idx].remotePort = 0;
  }

  // Responsibility 1: Start the usb connection for diagnostics
  // This is normally Serial but uses SerialUSB on a SAMD processor
  Serial.begin(115200);
  // while (!Serial.available())
  //   ;
  delay(500);

  DIAG(F("DCC++ EX v%S"),F(VERSION));

  CONDITIONAL_LCD_START {
    // This block is ignored if LCD not in use
    LCD(0,F("DCC++ EX v%S"),F(VERSION));
    LCD(1,F("Starting"));
    }

  //  Start the WiFi interface on a MEGA, Uno cannot currently handle WiFi
#if WIFI_ON
  WifiInterface::setup(WIFI_SERIAL_LINK_SPEED, F(WIFI_SSID), F(WIFI_PASSWORD), F(WIFI_HOSTNAME), IP_PORT);
#endif // WIFI_ON

#if ETHERNET_ON
  EthernetInterface::setup();
#endif // ETHERNET_ON

  // Responsibility 3: Start the DCC engine.
  // Note: this provides DCC with two motor drivers, main and prog, which handle the motor shield(s)
  // Standard supported devices have pre-configured macros but custome hardware installations require
  //  detailed pin mappings and may also require modified subclasses of the MotorDriver to implement specialist logic.

  main_track.begin();
  prog_track.begin();

  DCC::begin(
    F("STANDARD_MOTOR_SHIELD"),
    &main_track, &prog_track);


  Serial.println(F("End DCC::begin -- All ready"));
  // Configure and connect to Wifi
  LCD(1,F("Connecting wifi..."));
  setupWifi();

  // start the server:
  server = new WiFiServer(80);
  server->begin();
  Serial.println(F("wifi Server started"));

  printFreeMem();
  Serial.println(F("Startup Complete"));
  LCD(1,F("Ready"));

  catseye_red.begin();
  catseye_blue.begin();
  quick_timer.begin();
  slow_timer.begin();

  catseye_blue.flipPolarity();
  catseye_red.flipPolarity();

  catseye_red.turnOff();
  catseye_blue.turnOff();

  pinMode(CATSEYE_PRESS_PIN, INPUT);
}

void loop()
{
  static int last_press_state = 0;

  // The main sketch has responsibilities during loop()

  // Responsibility 1: Handle DCC background processes
  //                   (loco reminders and power checks)
  DCC::loop();

  // Responsibility 2: handle any incoming commands on USB connection
  serialParser.loop(Serial);

// Responsibility 3: Optionally handle any incoming WiFi traffic
#if WIFI_ON
  WifiInterface::loop();
#endif
#if ETHERNET_ON
  EthernetInterface::loop();
#endif

  LCDDisplay::loop();  // ignored if LCD not in use

  portParserLoop(server, &withrottle_sessions);

  // if(quick_timer.loop())
  // {
  //   catseye_blue.toggleOnOff();
  // }

  if(slow_timer.loop())
  {
    catseye_red.toggleOnOff();
  }

  int new_press_state = digitalRead(CATSEYE_PRESS_PIN);
  if(last_press_state != new_press_state)
  {
      catseye_blue.toggleOnOff();
  }
  last_press_state = new_press_state;
  catseye_red.loop();
  catseye_blue.loop();

// Optionally report any decrease in memory (will automatically trigger on first call)
#if ENABLE_FREE_MEM_WARNING
  static int ramLowWatermark = 32767; // replaced on first loop

  int freeNow = freeMemory();
  if (freeNow < ramLowWatermark)
  {
    ramLowWatermark = freeNow;
    LCD(2,F("Free RAM=%5db"), ramLowWatermark);
  }
#endif
}
