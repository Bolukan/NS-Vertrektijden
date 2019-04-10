#if !defined(ESP8266)
#error This file is for ESP8266 only
#endif

#include <Arduino.h>
#include <ESP8266WiFi.h>                // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <WiFiClientSecure.h>           // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h
#include <time.h>                       // Arduino library - Secure connections need the right time to know validity of certificate.

#include "apiportal_ns_nl.h"            // public key of apiportal.ns.nl and DigiCert Global Root CA

#include <U8g2lib.h>                    // https://github.com/olikraus/u8g2

#include "nsapi.h"
#include "secrets.h"                    // Uncomment this line and add secrets.h to your project
#ifndef SECRETS_H
 #define SECRETS_H
 const char WIFI_SSID[]                 = "ssid";
 const char WIFI_PASSWORD[]             = "password";
 const char OCP_APIM_SUBSCRIPTION_KEY[] = "12345678901234567890123456789012";
#endif

// ***********************************  CONSTANTS  *****************************

// time
static const int  TIME_TIMEZONE_NL           = 3600;
static const int  TIME_DAYLIGHTOFFSET_SEC_NL = 3600;
static const char TIME_NTPSERVER_1[]         = "nl.pool.ntp.org";
static const char TIME_NTPSERVER_2[]         = "pool.ntp.org";

NSAPI nsapi(OCP_APIM_SUBSCRIPTION_KEY);
String body;

// ***********************************  CONSTANTS  *****************************

bool geturl = true;

// ***********************************  WIFI  **********************************

// More events: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiGeneric.h

void onSTAConnected(WiFiEventStationModeConnected e /*String ssid, uint8 bssid[6], uint8 channel*/) {
  Serial.printf("WiFi Connected: SSID %s @ BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Channel %d\n",
    e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);
 }

void onSTADisconnected(WiFiEventStationModeDisconnected e /*String ssid, uint8 bssid[6], WiFiDisconnectReason reason*/) {
  Serial.printf("WiFi Disconnected: SSID %s BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Reason %d\n",
    e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.reason);
  // Reason: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
}

void onSTAGotIP(WiFiEventStationModeGotIP e /*IPAddress ip, IPAddress mask, IPAddress gw*/) {
  Serial.printf("WiFi GotIP: localIP %s SubnetMask %s GatewayIP %s\n",
    e.ip.toString().c_str(), e.mask.toString().c_str(), e.gw.toString().c_str());
}

// ***********************************  SETUP  *********************************

void setup(){
  // WiFi
  static WiFiEventHandler e1, e2, e4;

  Serial.begin(115200);
  /*
  other setup code
  */

  // WiFi events
  e1 = WiFi.onStationModeConnected(onSTAConnected);
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
  e4 = WiFi.onStationModeGotIP(onSTAGotIP);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(false);           // do not automatically connect on power on to the last used access point
  WiFi.setAutoReconnect(true);          // attempt to reconnect to an access point in case it is disconnected
  WiFi.persistent(false);               // Store no SSID/PASSWORD in flash
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // timezone CET = 3600, daylight CET->CEST = 3600
  // void configTime(int timezone, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
  configTime(TIME_TIMEZONE_NL, TIME_DAYLIGHTOFFSET_SEC_NL, TIME_NTPSERVER_1, TIME_NTPSERVER_2);

}

// ********************  LOOP  ********************

void loop()
{
  time_t now = time(nullptr);
  if ((now > 24 * 3600) & (geturl)) {
    geturl = false;

    BearSSL::WiFiClientSecure client;
    BearSSL::X509List cert(CERTIFICATE_ROOT);
    client.setTrustAnchors(&cert);
    body = String("");
    nsapi.fetchURL(&client, "htnc", &body);
    Serial.println(body);
    // fetchURL(&client, HOST_NS, PORT_NS, PATH_NS);
  }
}
