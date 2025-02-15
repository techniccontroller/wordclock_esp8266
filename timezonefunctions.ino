#include <ESP8266HTTPClient.h>
#include <AceTime.h>            // https://github.com/bxparks/AceTime
#include "ntp_client_plus.h"
#include "udplogger.h"

using namespace ace_time;

static const int CACHE_SIZE = 1;
static ExtendedZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static ExtendedZoneManager manager(
    zonedbx::kZoneAndLinkRegistrySize,
    zonedbx::kZoneAndLinkRegistry,
    zoneProcessorCache);

/**
 * @brief Request the timezone as string from the IP-API
 * 
 * @param logger UDPLogger object to log messages
 * @return String timezone string
 */
String getTimeZoneString(UDPLogger &logger) {
  WiFiClient client;
  HTTPClient http;
  logger.logString("[HTTP] Requesting timezone from IP-API");
  if (http.begin(client, "http://ip-api.com/json/")) { 
    int httpCode = http.GET();

    if (httpCode > 0) { 
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();

        int tzIndex = payload.indexOf("\"timezone\":\"");
        if (tzIndex != -1) {
          int tzStart = tzIndex + 12; 
          int tzEnd = payload.indexOf("\"", tzStart);
          String timezone = payload.substring(tzStart, tzEnd);
          logger.logString("[HTTP] Received timezone: " + timezone);
          return timezone;
        }
      }
    } 
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end(); // Close connection
  }
  else {
    logger.logString("[HTTP] Unable to connect");
  }
  return "";
}

/**
 * @brief Update the UTC offset from the timezone string obtained from the IP-API
 * 
 * @param logger UDPLogger object to log messages
 * @param ntp NTPClientPlus object to set the UTC offset
 * @return int 
 */
void updateUTCOffsetFromTimezoneAPI(UDPLogger &logger, NTPClientPlus &ntp) {
  String timezone = getTimeZoneString(logger);
  if (timezone.length() > 0) {
    uint16_t zone_index = manager.indexForZoneName(timezone.c_str());
    if (zone_index != ZoneManager::kInvalidIndex) {
      ExtendedZone zone = manager.getZoneForIndex(zone_index);
      int offset = zone.stdOffset().toMinutes();
      logger.logString("[ZoneManager] Timezone offset (min): " + String(offset));
      ntp.setUTCOffset(offset);
    }
    else {
      logger.logString("[ZoneManager] Error: Could not find time_zone value in DB. Use hardcoded UTC offset.");
    }       
  }
}