#ifndef ledcalendar_h
#define ledcalendar_h

#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs:
#include "udplogger.h"

#define CAL_LED_COUNT 50        // number of LEDs in the calendar strip (7 days + 31 dates + 12 months)
#define CAL_CURRENT_LIMIT 300   // limit the total current consumed by LEDs (mA)

// LED ranges for calendar sections
#define CAL_DAYOFWEEK_START 0   // First LED for day of week (Monday)
#define CAL_DAYOFWEEK_COUNT 7   // 7 LEDs for days of week (Mon-Sun)
#define CAL_DAYOFMONTH_START 7  // First LED for day of month (1st)
#define CAL_DAYOFMONTH_COUNT 31 // 31 LEDs for days of month (1-31)
#define CAL_MONTH_START 38      // First LED for month (January)
#define CAL_MONTH_COUNT 12      // 12 LEDs for months (Jan-Dec)

class LEDCalendar {
    public:
        LEDCalendar(Adafruit_NeoPixel *strip, UDPLogger *logger);
        static uint32_t Color24bit(uint8_t r, uint8_t g, uint8_t b);
        static uint32_t Wheel(uint8_t WheelPos);
        static uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor);

        void setupCalendar();
        void setOffset(int offset);

        void setBrightness(uint8_t brightness);
        void setCurrentLimit(uint16_t currentLimit);

        uint8_t getBrightness();

        void flushCalendar();

        void setPixel(uint16_t pixel, uint32_t color);
        void fill(uint32_t color);

        void setDayOfWeek(uint8_t dayOfWeek, uint32_t color);
        void setDayOfMonth(uint8_t dayOfMonth, uint32_t color);
        void setMonth(uint8_t month, uint32_t color);

        void setDate(uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint32_t color);

        void drawOnCalendarInstant();
        void drawOnCalendarSmooth(float factor);

    private:
        Adafruit_NeoPixel *strip;
        UDPLogger *logger;

        uint16_t currentLimit;
        uint8_t brightness;

        int offset;

        uint32_t targetStrip[CAL_LED_COUNT] = {0};
        uint32_t currentStrip[CAL_LED_COUNT] = {0};

        void drawOnCalendar(float factor);
        uint16_t calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness);
};

#endif
