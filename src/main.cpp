#if !defined(ESP8266)
#error This file is for ESP8266 only
#endif

#define COMPDATE __DATE__ __TIME__
#define APPNAME "NS-Vertrektijden"
#define VERSION "V0.0.1"

#include <Arduino.h>
#include <ESP8266WiFi.h>                // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFi.h
#include <WiFiClientSecure.h>           // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h
#include <time.h>                       // Arduino library - Secure connections need the right time to know validity of certificate.

#define ARDUINOJSON_DECODE_UNICODE 1
#include <ArduinoJson.h>

// Connect ESP8266 with SSD1306 OLED, SPI version (7 pins)
// IO = D1-mini = description  => OLED = pin
//       G = Ground            => GND = 1
//     3V3 = Power             => VCC = 2
// 14 = D5 = HSPI Clock        => D0  = 3
// 13 = D7 = HSPI Data MOSI    => D1  = 4
//  2 = D4 = Reset             => RES = 5
// 16 = D0 = Data/Command      => DC  = 6
// 15 = D8 = Chip select       => CS  = 7
// 12 = D6 = HSPI Data MISO    KEEP UNCONNECTED

#include <U8g2lib.h>                    // https://github.com/olikraus/u8g2
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(/* rotation= */ U8G2_R2, /* cs = */ 15, /* dc = */ 16, /* reset = */ 2);
// U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R2, /* clock=*/ 14, /* data=*/ 13, /* cs=*/ 15, /* dc=*/ 16, /* reset=*/ 2);

#include "apiportal_ns_nl.h"            // public key of apiportal.ns.nl and DigiCert Global Root CA
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
char* body;

// ***********************************  CONSTANTS  *****************************

bool geturl = true;
char bufferTijd[5][9] = { "23:59:00", "23:59:00", "23:59:00", "23:59:00", "23:59:00" };
int loopstart;

// ***********************************  FUNCTIONS  *****************************
void getXMLValueOfKey(
  String line,
  String &value,
  const char* key,
  int startPos = 0)
{
  String startKey = "<"; startKey += key; startKey += ">";
  String endKey = "</";  endKey += key;   endKey += ">";
  int start = line.indexOf(startKey, startPos);
  if (start > -1) {
    int end  = line.indexOf(endKey, start);
    value = line.substring(start + startKey.length(), end);
  }
}


void show4rows(const char* row1, const char* row2, const char* row3, const char* row4) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR12_te);
    u8g2.drawStr(10, 15, row1);
    u8g2.drawStr(10, 31, row2);
    u8g2.drawStr(10, 47, row3);
    u8g2.drawStr(10, 47, row4);
  } while ( u8g2.nextPage() );
}

void showWiFi(const char* status, const char* row3) {
  show4rows("Starting WiFi", status, row3, "");
}

void showTimes() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_helvR12_te);
    u8g2.drawStr(10, 15, bufferTijd[0]);
    u8g2.drawStr(10, 31, bufferTijd[1]);
    u8g2.drawStr(10, 47, bufferTijd[2]);
    u8g2.drawStr(10, 63, bufferTijd[3]);
    u8g2.drawStr(60, 15, "");
    u8g2.drawStr(60, 31, "");
    u8g2.drawStr(60, 47, "");
    u8g2.drawStr(60, 63, "");
  } while ( u8g2.nextPage() );

}

// ***********************************  WIFI  **********************************

// More events: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiGeneric.h

void onSTAConnected(WiFiEventStationModeConnected e /*String ssid, uint8 bssid[6], uint8 channel*/) {
  Serial.printf("WiFi Connected: SSID %s @ BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Channel %d\n",
    e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.channel);
    showWiFi("Get IP from", e.ssid.c_str());
 }

void onSTADisconnected(WiFiEventStationModeDisconnected e /*String ssid, uint8 bssid[6], WiFiDisconnectReason reason*/) {
  Serial.printf("WiFi Disconnected: SSID %s BSSID %.2X:%.2X:%.2X:%.2X:%.2X:%.2X Reason %d\n",
    e.ssid.c_str(), e.bssid[0], e.bssid[1], e.bssid[2], e.bssid[3], e.bssid[4], e.bssid[5], e.reason);
  // Reason: https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/ESP8266WiFiType.h
  showWiFi("ERROR", e.ssid.c_str());
}

void onSTAGotIP(WiFiEventStationModeGotIP e /*IPAddress ip, IPAddress mask, IPAddress gw*/) {
  Serial.printf("WiFi GotIP: localIP %s SubnetMask %s GatewayIP %s\n",
    e.ip.toString().c_str(), e.mask.toString().c_str(), e.gw.toString().c_str());
    showWiFi("Got IP", e.ip.toString().c_str());
}

// ***********************************  SETUP  *********************************

void setup(){
  // WiFi
  static WiFiEventHandler e1, e2, e4;

  Serial.begin(115200); Serial.println();

  // Screen
  u8g2.begin();

  // WiFi events
  e1 = WiFi.onStationModeConnected(onSTAConnected);
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
  e4 = WiFi.onStationModeGotIP(onSTAGotIP);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoConnect(false);           // do not automatically connect on power on to the last used access point
  WiFi.setAutoReconnect(true);          // attempt to reconnect to an access point in case it is disconnected
  WiFi.persistent(false);               // Store no SSID/PASSWORD in flash
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  showWiFi("Connect to ...", WIFI_SSID);

  // timezone CET = 3600, daylight CET->CEST = 3600
  // void configTime(int timezone, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
  configTime(TIME_TIMEZONE_NL, TIME_DAYLIGHTOFFSET_SEC_NL, TIME_NTPSERVER_1, TIME_NTPSERVER_2);

  loopstart = 99;
}

// ********************  LOOP  ********************

void loop()
{
  time_t now = time(nullptr);

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  if ((now > 24 * 3600) & (loopstart != timeinfo.tm_min))
  {
    loopstart = timeinfo.tm_min;

//    show4rows("NS API", "fetch data", "", "");
    BearSSL::WiFiClientSecure client;
    BearSSL::X509List cert(CERTIFICATE_ROOT);
    client.setTrustAnchors(&cert);
    NSAPI::NSAPI_Error error = nsapi.fetchURL(&client, "htnc", body);

    // Serial.println(body);
    if (error == NSAPI::NSAPI_Error_None) {
      const size_t capacity = 10000;  // Stats: 10 -> 4048 bytes
      DynamicJsonDocument doc(capacity);
      DeserializationError err = deserializeJson(doc, body);
      if (!err) {
        int sizearr = doc["payload"]["departures"].size();
        int count = 0;
        
        Serial.print(sizearr);
        Serial.print(" departures. JSON uses ");
        Serial.print(doc.memoryUsage());
        Serial.print(" bytes\n");

        for (int i = 0; i < sizearr; i++) {
          const char* direction = doc["payload"]["departures"][i]["direction"].as<char*>();
          if ((strcmp(direction, "'s-Hertogenbosch")  == 0) |
              (strcmp(direction, "Tiel")  == 0))
          {
            //
          } else {
            char actualDateTime[9];
            memcpy ((void *)&actualDateTime, (void *)&(doc["payload"]["departures"][i]["actualDateTime"].as<const char*>()[11]), 8);
            actualDateTime[8] = 0;

            char plannedDateTime[9];
            memcpy ((void *)&plannedDateTime, (void *)&(doc["payload"]["departures"][i]["plannedDateTime"].as<const char*>()[11]), 8);
            plannedDateTime[8] = 0;

            if (count < 5) {
              if (strcmp(actualDateTime, plannedDateTime) == 0)
              {
                memcpy ((void *)&bufferTijd[count], (void *)&(doc["payload"]["departures"][i]["plannedDateTime"].as<const char*>()[11]), 5);
                bufferTijd[count][5] = 0;
              } else {
                memcpy ((void *)&bufferTijd[count], (void *)&(doc["payload"]["departures"][i]["actualDateTime"].as<const char*>()[11]), 8);
                bufferTijd[count][8] = 0;
              }
              count++;
            }

            Serial.print(actualDateTime);
            Serial.print(" ");
            Serial.print(direction);
            Serial.println();
          }
        }
        showTimes();
      } else {
        Serial.print("Error: "); Serial.println(err.c_str());
      }
    }
    free(body);
  }
}
