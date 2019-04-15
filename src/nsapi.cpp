#include <Arduino.h>
#include <nsapi.h>

// time
const char NSAPI::HOST_NS[]             = "gateway.apiportal.ns.nl";
const uint16_t NSAPI::PORT_NS           = 443;
const char NSAPI::PATH_NS[]             = "/public-reisinformatie/api/v2/departures?maxJourneys=%d&lang=nl&station=%s";

NSAPI::NSAPI(const char* OcpApimSubscriptionKey) :
  _OcpApimSubscriptionKey(OcpApimSubscriptionKey)
{ }

// ***********************************  TIME  **********************************

NSAPI::NSAPI_Error NSAPI::fetchURL(
  BearSSL::WiFiClientSecure *client,
  const char *station,
  char*& body)
{
  bool finishedHeaders = false;
  bool gotResponse = false;
  String headers = "";
  String line = "";
  char path[100];
  char c;
  uint32_t contentLength = 0;
  uint32_t position = 0;

  sprintf(path, PATH_NS, MAX_JOURNEYS, station);
  // Serial.println(path);

  if (!client->connect(HOST_NS, PORT_NS)) {
    return NSAPI_Error_NoConnection;
  }

  client->printf("GET %s HTTP/1.1\r\n", path);
  client->printf("Host: %s\r\n", HOST_NS);
  client->write("User-Agent: ESP8266\r\n");
  client->printf("Ocp-Apim-Subscription-Key: %s\r\n", _OcpApimSubscriptionKey);
  client->write("Connection: close\r\n");
  client->write("\r\n");

  uint32_t timeout = millis() + 2000;

  // checking the timeout
  while (millis() < timeout) {
    while (client->available()) {
      c = client->read();
      // body
      if (finishedHeaders) {
        body[position++] = c;
        if (position == contentLength) timeout = millis();
      } else {
      // header
        // eol
        if (c == '\n') {
          if (line == "\r") {
            // end of header
            finishedHeaders = true;
            // Serial.println(headers);
          } else
          if (line.indexOf("Content-Length") > -1) {
            // Content-Length
            contentLength = line.substring(16, line.length()-1).toInt();
            if (contentLength > MAX_SIZE_RETURN_STRING) {
              return NSAPI_Error_ResponseToLong;
            }
            body = (char*)malloc(contentLength+1);
          }
          headers.concat(line);
          headers.concat("\n");
          line = "";
        } else {
          // not eol
          line = line + c;
        }
      }

      gotResponse = true;
    }
  }

  if (!gotResponse) {
    return NSAPI_Error_NoResponse;
  }

  client->stop();
  body[position] = 0;
  return NSAPI_Error_None;
}
