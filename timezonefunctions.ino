#include <ESP8266HTTPClient.h>
#include "ntp_client_plus.h"
#include "udplogger.h"

int api_offset = 0;
String api_timezone = "";
float api_lat = 0.0;
float api_lon = 0.0;

/**
 * @brief Request the timezone and other data from the IP-API
 * 
 * @param logger UDPLogger object to log messages
 * @return bool true if the api request was successful
 */
bool requestAPIData(UDPLogger &logger) {
  WiFiClient client;
  HTTPClient http;
  bool res = false;
  logger.logString("[HTTP] Requesting timezone from IP-API");
  // see API documentation on https://ip-api.com/docs/api:json to see which fields are available
  if (http.begin(client, "http://ip-api.com/json/?fields=status,message,country,countryCode,region,regionName,city,zip,lat,lon,timezone,offset,query")) { 
    int httpCode = http.GET();

    if (httpCode > 0) { 
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();

        api_timezone = getJsonParameterValue(payload, "timezone", true);
        logger.logString("[HTTP] Received timezone: " + api_timezone);

        String offsetString = getJsonParameterValue(payload, "offset", false);
        api_offset = offsetString.toInt() / 60;
        logger.logString("[HTTP] Received offset (min): " + String(api_offset));

        String latString = getJsonParameterValue(payload, "lat", false);
        api_lat = latString.toFloat();
        logger.logString("[HTTP] Received latitude: " + String(api_lat));

        String lonString = getJsonParameterValue(payload, "lon", false);
        api_lon = lonString.toFloat();
        logger.logString("[HTTP] Received longitude: " + String(api_lon));

      }
    } 
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      res = false;
    }
    http.end(); // Close connection
  }
  else {
    logger.logString("[HTTP] Unable to connect");
    res = false;
  }
  return res;
}

/**
 * @brief Get the Json Parameter Value object
 * 
 * @param json 
 * @param parameter 
 * @return String 
 */
String getJsonParameterValue(String json, String parameter, bool isString) {
  String value = "";
  if(isString) {
    int index = json.indexOf("\"" + parameter + "\":\"");
    if (index != -1) {
      int start = index + parameter.length() + 4; 
      int end = json.indexOf("\"", start);
      value = json.substring(start, end);
    }
  }
  else {
    int index = json.indexOf("\"" + parameter + "\":");
    if (index != -1) {
      int start = index + parameter.length() + 3; 
      int end = json.indexOf(",", start);
      value = json.substring(start, end);
    }
  }
  return value;
}

/**
 * @brief Update the UTC offset from the timezone string obtained from the IP-API
 * 
 * @param logger UDPLogger object to log messages
 * @param ntp NTPClientPlus object to set the UTC offset
 * @return int 
 */
void updateUTCOffsetFromTimezoneAPI(UDPLogger &logger, NTPClientPlus &ntp) {
  bool res = requestAPIData(logger);
  if (res) {
    ntp.setUTCOffset(api_offset);      
  }
}