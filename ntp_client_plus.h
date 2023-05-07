#ifndef ntpclientplus_h
#define ntpclientplus_h

#include <Arduino.h>
#include <WiFiUdp.h>

#define SEVENZYYEARS 2208988800UL
#define NTP_PACKET_SIZE 48
#define NTP_DEFAULT_LOCAL_PORT 1337

/**
 * @brief Own NTP Client library for Arduino with code from:
 * - https://github.com/arduino-libraries/NTPClient
 * - SPS&Technik - Projekt WordClock v1.02
 * 
 */
class NTPClientPlus{

    public:
        NTPClientPlus(UDP &udp, const char* poolServerName, int utcx, bool _swChange);
        void setupNTPClient();
        int updateNTP();
        void end();
        void setTimeOffset(int timeOffset);
        void setPoolServerName(const char* poolServerName);
        unsigned long getSecsSince1900() const;
        unsigned long getEpochTime() const;
        int getHours24() const;
        int getHours12() const;
        int getMinutes() const;
        int getSeconds() const;
        String getFormattedTime() const;
        String getFormattedDate();
        void calcDate();
        unsigned int getDayOfWeek();
        unsigned int getYear();
        bool isLeapYear(unsigned int year);
        int getMonth(int dayOfYear);
        long getTimeOffset();
        bool updateSWChange();


    private:
        UDP*          _udp;
        bool          _udpSetup       = false;

        const char*   _poolServerName = "pool.ntp.org"; // Default time server
        IPAddress     _poolServerIP;
        unsigned int  _port           = NTP_DEFAULT_LOCAL_PORT;
        long          _timeOffset     = 0;
        int           _utcx           = 0;
        bool          _swChange        = 1;

        unsigned long _updateInterval = 60000;  // In ms

        unsigned long _currentEpoc    = 0;      // In s
        unsigned long _lastUpdate     = 0;      // In ms
        unsigned long _secsSince1900  = 0;      // seconds since 1. Januar 1900, 00:00:00
        unsigned long _lastSecsSince1900 = 0;
        unsigned int _dateYear         = 0;
        unsigned int _dateMonth        = 0;
        unsigned int _dateDay          = 0;
        unsigned int _dayOfWeek        = 0;


        byte          _packetBuffer[NTP_PACKET_SIZE];
        void          sendNTPPacket();
        void          setSummertime(bool summertime);
        

        static const unsigned long secondperday = 86400;
        static const unsigned long secondperhour = 3600;
        static const unsigned long secondperminute = 60;
        static const unsigned long minuteperhour = 60;
        static const unsigned long millisecondpersecond = 1000;

        // number of days in months
        unsigned int daysInMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};




};




#endif