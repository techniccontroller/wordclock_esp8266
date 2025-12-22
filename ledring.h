#ifndef ledring_h
#define ledring_h

#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs:
#include "udplogger.h"

#define RING_LED_COUNT 60       // number of LEDs in the ring
#define RING_CURRENT_LIMIT 1000 // limit the total current sonsumed by LEDs (mA)

class LEDRing {
    public:
        LEDRing(Adafruit_NeoPixel *ring, UDPLogger *logger);
        static uint32_t Color24bit(uint8_t r, uint8_t g, uint8_t b);
        static uint32_t Wheel(uint8_t WheelPos);
        static uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor);

        void setupRing();
        void setOffset(int offset);

        void setBrightness(uint8_t brightness);
        void setCurrentLimit(uint16_t currentLimit);

        uint8_t getBrightness();

        void flushRing();

        void setPixel(uint16_t pixel, uint32_t color);
        void fill(uint32_t color);

        void drawOnRingInstant();
        void drawOnRingSmooth(float factor);

    private:
        Adafruit_NeoPixel *ring;
        UDPLogger *logger;

        uint16_t currentLimit;
        uint8_t brightness;

        int offset;

        uint32_t targetRing[RING_LED_COUNT] = {0};
        uint32_t currentRing[RING_LED_COUNT] = {0};

        void drawOnRing(float factor);
        uint16_t calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness);
};

#endif
