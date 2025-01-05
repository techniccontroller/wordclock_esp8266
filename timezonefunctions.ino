#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <AceTime.h>
#include "ntp_client_plus.h"
#include "udplogger.h"

// URL for timezone API
const char *url = "https://js.maxmind.com/geoip/v2.1/city/me?referrer=https%3A%2F%2Fwww.maxmind.com";

using namespace ace_time;

static const int CACHE_SIZE = 1;
static ExtendedZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static ExtendedZoneManager manager(
    zonedbx::kZoneAndLinkRegistrySize,
    zonedbx::kZoneAndLinkRegistry,
    zoneProcessorCache);

/**
 * @brief Get the Timezone as string from the MaxMind API
 * 
 * @param logger UDPLogger object to log messages
 * @return String timezone string
 */
String getTimezoneString(UDPLogger &logger) {
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;
  logger.logString("[HTTPS] Requesting timezone from MaxMind API");
  if (https.begin(*client, url)) { 
    int httpCode = https.GET();

    if (httpCode > 0) { 
      if (httpCode == HTTP_CODE_OK) {
        String payload = https.getString();
        logger.logString("[HTTPS] Response received, length: " + String(payload.length()));

        // Search for "time_zone" in the response
        int index = payload.indexOf("time_zone");
        if (index != -1) {
          int startIndex = payload.indexOf('"', index + 10) + 1; // Find the first '"' after "time_zone"
          int endIndex = payload.indexOf('"', startIndex);       // Find the closing '"'
          if (startIndex != -1 && endIndex != -1) {
            String timeZone = payload.substring(startIndex, endIndex);
            logger.logString("[HTTPS] Extracted time_zone: " + timeZone);
            return timeZone;
          }
          else {
            logger.logString("[HTTPS] Error: Could not extract time_zone value.");
          }
        }
        else {
          logger.logString("[HTTPS] Error: time_zone not found in response.");
        }
      }
    }
    else {
      logger.logString("[HTTPS] request failed, error: " + https.errorToString(httpCode));
    }
    https.end(); // Close connection
  }
  else {
    logger.logString("[HTTPS] Unable to connect");
  }

  return "";
}

/**
 * @brief Update the UTC offset from the timezone string obtained from the MaxMind API
 * 
 * @param logger UDPLogger object to log messages
 * @param ntp NTPClientPlus object to set the UTC offset
 * @return int 
 */
void updateUTCOffsetFromTimezoneAPI(UDPLogger &logger, NTPClientPlus &ntp) {
  String timezone = getTimezoneString(logger);
  if (timezone.length() > 0) {
    uint16_t zone_index = manager.indexForZoneName(timezone.c_str());
    if (zone_index != ZoneManager::kInvalidIndex) {
      ExtendedZone zone = manager.getZoneForIndex(zone_index);
      int offset = zone.stdOffset().toMinutes();
      logger.logString("[HTTPS] Timezone offset: " + String(offset));
      ntp.setUTCOffset(offset);
    }
    else {
      logger.logString("[HTTPS] Error: Could not find time_zone value in DB. Use hardcoded UTC offset.");
    }       
  }
}