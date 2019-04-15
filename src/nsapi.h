#ifndef NSAPI_H
#define NSAPI_H

#include <Arduino.h>
#include <WiFiClientSecure.h>           // Arduino library - https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/src/WiFiClientSecure.h
#include "apiportal_ns_nl.h"            // public key of apiportal.ns.nl and DigiCert Global Root CA

class NSAPI
{
public:

  enum NSAPI_Error {
    NSAPI_Error_None           = 0,
    NSAPI_Error_NoConnection   = 1,
    NSAPI_Error_ResponseToLong = 2,
    NSAPI_Error_NoResponse     = 3,
  };

  static const uint32_t MAX_SIZE_RETURN_STRING = 0x00004000; // 16KB
  static const uint32_t MAX_SIZE_RETURN_SPIFFS = 0x00080000; // 512KB
  static const uint16_t MAX_JOURNEYS           = 1;

  // constructor
  NSAPI(const char* OcpApimSubscriptionKey);
  NSAPI_Error fetchURL(BearSSL::WiFiClientSecure *client, const char *station, char*& body);

private:
  static const char     HOST_NS[]       ;
  static const uint16_t PORT_NS         ;
  static const char     PATH_NS[]       ;

  const char* _OcpApimSubscriptionKey;

};

#endif
