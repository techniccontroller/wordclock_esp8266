#include "ledring.h"

LEDRing::LEDRing(Adafruit_NeoPixel *ring, UDPLogger *logger){
    this->ring = ring;
    this->logger = logger;
    this->currentLimit = RING_CURRENT_LIMIT;
    this->brightness = 255;
}

/**
 * @brief Convert RGB value to 24bit color value
 * 
 * @param r red value (0-255)
 * @param g green value (0-255)
 * @param b blue value (0-255)
 * @return uint32_t 24bit color value
 */
uint32_t LEDRing::Color24bit(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

/**
 * @brief Input a value 0 to 255 to get a color value. The colors are a transition r - g - b - back to r.
 * 
 * @param WheelPos Value between 0 and 255
 * @return uint32_t return 24bit color of colorwheel
 */
uint32_t LEDRing::Wheel(uint8_t WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return Color24bit(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return Color24bit(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Color24bit(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**
 * @brief Interpolates two colors24bit and returns an color of the result
 * 
 * @param color1 startcolor for interpolation
 * @param color2 endcolor for interpolation
 * @param factor which color is wanted on the path from start to end color
 * @return uint32_t interpolated color
 */
uint32_t LEDRing::interpolateColor24bit(uint32_t color1, uint32_t color2, float factor){
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + (r2 - r1) * factor;
    uint8_t g = g1 + (g2 - g1) * factor;
    uint8_t b = b1 + (b2 - b1) * factor;

    return Color24bit(r, g, b);
}

/**
 * @brief Setup the LED ring
 * 
*/
void LEDRing::setupRing(){
    ring->begin();
    ring->setBrightness(this->brightness);
}

/**
 * @brief Set the offset of the ring
 * 
 * @param offset offset of the ring
 */
void LEDRing::setOffset(int offset){
    this->offset = offset;
}

/**
 * @brief Set the brightness of the ring
 * 
 * @param brightness brightness value (0-255)
 */
void LEDRing::setBrightness(uint8_t brightness){
    this->brightness = brightness;
    ring->setBrightness(brightness);
}

/**
 * @brief Set the current limit of the LED ring
 * 
 * @param currentLimit current limit value (0-9999)
 */
void LEDRing::setCurrentLimit(uint16_t currentLimit){
    this->currentLimit = currentLimit;
}

/**
 * @brief Get the brightness of the ring
 * 
 * @return uint8_t brightness value (0-255)
 */
uint8_t LEDRing::getBrightness(){
    return this->brightness;
}

/**
 * @brief Flush the ring
 * 
 */
void LEDRing::flushRing(){
    // set all pixels to black
    for(int i=0; i<RING_LED_COUNT; i++) {
        targetRing[i] = 0;
    }
}

/**
 * @brief Set the color of a pixel on the ring
 * 
 * @param pixel pixel number
 * @param color 24bit color value
 */
void LEDRing::setPixel(uint16_t pixel, uint32_t color){
    // check if pixel is in range
    if(pixel >= RING_LED_COUNT){
        logger->logString("ERROR: pixel out of range");
        return;
    }
    targetRing[pixel] = color;
}

/**
 * @brief Fill the ring with a color
 * 
 * @param color 24bit color value
 */
void LEDRing::fill(uint32_t color)
{
    // set all pixels to the same color
    for(int i=0; i<RING_LED_COUNT; i++) {
        targetRing[i] = color;
    }
}

/**
 * @brief Draw the target color on the ring instantly
 * 
 */
void LEDRing::drawOnRingInstant(){
    drawOnRing(1.0);
}

/**
 * @brief Draw the target color on the ring with a smooth transition
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDRing::drawOnRingSmooth(float factor){
    drawOnRing(factor);
}

/**
 * @brief Draw the target color on the ring
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDRing::drawOnRing(float factor){

    // set pixels on ring and calculate current
    uint16_t totalCurrent = 0;
    for(int i=0; i<RING_LED_COUNT; i++) {
        uint32_t currentColor = currentRing[i];
        uint32_t targetColor = targetRing[i];
        uint32_t newColor = interpolateColor24bit(currentColor, targetColor, factor);
        int correctedPixel = (RING_LED_COUNT + i + offset) % RING_LED_COUNT;
        ring->setPixelColor(correctedPixel, newColor);
        currentRing[i] = newColor;

        totalCurrent += calcEstimatedLEDCurrent(newColor, this->brightness);
    }

    // Check if totalCurrent reaches CURRENTLIMIT -> if yes reduce brightness
    if(totalCurrent > this->currentLimit){
        uint8_t newBrightness = this->brightness * float(this->currentLimit)/float(totalCurrent);
        ring->setBrightness(newBrightness);
    }
    else {
        ring->setBrightness(this->brightness);
    }

    // show ring
    ring->show();
}

/**
 * @brief Calc estimated current (mA) for one pixel with the given color and brightness
 * 
 * @param color 24bit color value of the pixel for which the current should be calculated
 * @param brightness brightness value (0-255)
 * @return the current in mA
 */
uint16_t LEDRing::calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness){
  // extract rgb values
  uint8_t red = color >> 16 & 0xff;
  uint8_t green = color >> 8 & 0xff;
  uint8_t blue = color & 0xff;
  
  // Linear estimation: 20mA for full brightness per LED 
  // (calculation avoids float numbers)
  uint32_t estimatedCurrent = (20 * red) + (20 * green) + (20 * blue);
  estimatedCurrent /= 255;
  estimatedCurrent = (estimatedCurrent * brightness)/255;

  return estimatedCurrent;
}
