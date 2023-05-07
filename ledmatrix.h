#ifndef ledmatrix_h
#define ledmatrix_h

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include "udplogger.h"

// width of the led matrix
#define WIDTH 11
// height of the led matrix
#define HEIGHT 11

#define DEFAULT_CURRENT_LIMIT 9999

class LEDMatrix{
    public:
        LEDMatrix(Adafruit_NeoMatrix *mymatrix, uint8_t mybrightness, UDPLogger *mylogger);
        static uint32_t Color24bit(uint8_t r, uint8_t g, uint8_t b);
        static uint16_t color24to16bit(uint32_t color24bit);
        static uint32_t Wheel(uint8_t WheelPos);
        static uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor);
        void setupMatrix();
        void setMinIndicator(uint8_t pattern, uint32_t color);
        void gridAddPixel(uint8_t x, uint8_t y, uint32_t color);
        void gridFlush(void);
        void drawOnMatrixInstant();
        void drawOnMatrixSmooth(float factor);
        void printNumber(uint8_t xpos, uint8_t ypos, uint8_t number, uint32_t color);
        void printChar(uint8_t xpos, uint8_t ypos, char character, uint32_t color);
        void setBrightness(uint8_t mybrightness);
        void setCurrentLimit(uint16_t mycurrentLimit);

    private:

        Adafruit_NeoMatrix *neomatrix;
        UDPLogger *logger;

        uint8_t brightness;
        uint16_t currentLimit;

        // target representation of matrix as 2D array
        uint32_t targetgrid[HEIGHT][WIDTH] = {0};

        // current representation of matrix as 2D array
        uint32_t currentgrid[HEIGHT][WIDTH] = {0};

        // target representation of minutes indicator leds
        uint32_t targetindicators[4] = {0, 0, 0, 0};

        // current representation of minutes indicator leds
        uint32_t currentindicators[4] = {0, 0, 0, 0};

        void drawOnMatrix(float factor);
        uint16_t calcEstimatedLEDCurrent(uint32_t color);


};

#endif