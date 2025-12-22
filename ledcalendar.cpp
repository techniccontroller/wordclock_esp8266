#include "ledcalendar.h"

LEDCalendar::LEDCalendar(Adafruit_NeoPixel *strip, UDPLogger *logger){
    this->strip = strip;
    this->logger = logger;
    this->currentLimit = CAL_CURRENT_LIMIT;
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
uint32_t LEDCalendar::Color24bit(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

/**
 * @brief Input a value 0 to 255 to get a color value. The colors are a transition r - g - b - back to r.
 * 
 * @param WheelPos Value between 0 and 255
 * @return uint32_t return 24bit color of colorwheel
 */
uint32_t LEDCalendar::Wheel(uint8_t WheelPos) {
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
uint32_t LEDCalendar::interpolateColor24bit(uint32_t color1, uint32_t color2, float factor){
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
 * @brief Setup the LED calendar strip
 * 
*/
void LEDCalendar::setupCalendar(){
    strip->begin();
    strip->setBrightness(this->brightness);
}

/**
 * @brief Set the offset of the calendar strip
 * 
 * @param offset offset of the strip
 */
void LEDCalendar::setOffset(int offset){
    this->offset = offset;
}

/**
 * @brief Set the brightness of the calendar strip
 * 
 * @param brightness brightness value (0-255)
 */
void LEDCalendar::setBrightness(uint8_t brightness){
    this->brightness = brightness;
    strip->setBrightness(brightness);
}

/**
 * @brief Set the current limit of the LED calendar strip
 * 
 * @param currentLimit current limit value (0-9999)
 */
void LEDCalendar::setCurrentLimit(uint16_t currentLimit){
    this->currentLimit = currentLimit;
}

/**
 * @brief Get the brightness of the calendar strip
 * 
 * @return uint8_t brightness value (0-255)
 */
uint8_t LEDCalendar::getBrightness(){
    return this->brightness;
}

/**
 * @brief Flush the calendar strip
 * 
 */
void LEDCalendar::flushCalendar(){
    // set all pixels to black
    for(int i=0; i<CAL_LED_COUNT; i++) {
        targetStrip[i] = 0;
    }
}

/**
 * @brief Set the color of a pixel on the calendar strip
 * 
 * @param pixel pixel number
 * @param color 24bit color value
 */
void LEDCalendar::setPixel(uint16_t pixel, uint32_t color){
    // check if pixel is in range
    if(pixel >= CAL_LED_COUNT){
        logger->logString("ERROR: pixel out of range");
        return;
    }
    targetStrip[pixel] = color;
}

/**
 * @brief Fill the calendar strip with a color
 * 
 * @param color 24bit color value
 */
void LEDCalendar::fill(uint32_t color)
{
    // set all pixels to the same color
    for(int i=0; i<CAL_LED_COUNT; i++) {
        targetStrip[i] = color;
    }
}

/**
 * @brief Set the LED for a specific day of week
 * 
 * @param dayOfWeek day of week (1=Monday, 2=Tuesday, ..., 7=Sunday)
 * @param color 24bit color value
 */
void LEDCalendar::setDayOfWeek(uint8_t dayOfWeek, uint32_t color){
    // check if dayOfWeek is in range (1-7)
    if(dayOfWeek < 1 || dayOfWeek > 7){
        logger->logString("ERROR: dayOfWeek out of range");
        return;
    }
    // dayOfWeek: 1=Monday corresponds to LED 0
    uint16_t pixel = CAL_DAYOFWEEK_START + (dayOfWeek - 1);
    targetStrip[pixel] = color;
}

/**
 * @brief Set the LED for a specific day of month
 * 
 * @param dayOfMonth day of month (1-31)
 * @param color 24bit color value
 */
void LEDCalendar::setDayOfMonth(uint8_t dayOfMonth, uint32_t color){
    // check if dayOfMonth is in range (1-31)
    if(dayOfMonth < 1 || dayOfMonth > 31){
        logger->logString("ERROR: dayOfMonth out of range");
        return;
    }
    // dayOfMonth: 1 corresponds to LED 7
    uint16_t pixel = CAL_DAYOFMONTH_START + (dayOfMonth - 1);
    targetStrip[pixel] = color;
}

/**
 * @brief Set the LED for a specific month
 * 
 * @param month month (1=January, 2=February, ..., 12=December)
 * @param color 24bit color value
 */
void LEDCalendar::setMonth(uint8_t month, uint32_t color){
    // check if month is in range (1-12)
    if(month < 1 || month > 12){
        logger->logString("ERROR: month out of range");
        return;
    }
    // month: 1=January corresponds to LED 38
    uint16_t pixel = CAL_MONTH_START + (month - 1);
    targetStrip[pixel] = color;
}

/**
 * @brief Set the LEDs for a complete date (day of week, day of month, and month)
 * 
 * @param dayOfWeek day of week (1=Monday, 2=Tuesday, ..., 7=Sunday)
 * @param dayOfMonth day of month (1-31)
 * @param month month (1=January, 2=February, ..., 12=December)
 * @param color 24bit color value
 */
void LEDCalendar::setDate(uint8_t dayOfWeek, uint8_t dayOfMonth, uint8_t month, uint32_t color){
    setDayOfWeek(dayOfWeek, color);
    setDayOfMonth(dayOfMonth, color);
    setMonth(month, color);
}

/**
 * @brief Draw the target color on the calendar strip instantly
 * 
 */
void LEDCalendar::drawOnCalendarInstant(){
    drawOnCalendar(1.0);
}

/**
 * @brief Draw the target color on the calendar strip with a smooth transition
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDCalendar::drawOnCalendarSmooth(float factor){
    drawOnCalendar(factor);
}

/**
 * @brief Draw the target color on the calendar strip
 * 
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDCalendar::drawOnCalendar(float factor){

    // set pixels on strip and calculate current
    uint16_t totalCurrent = 0;
    for(int i=0; i<CAL_LED_COUNT; i++) {
        uint32_t currentColor = currentStrip[i];
        uint32_t targetColor = targetStrip[i];
        uint32_t newColor = interpolateColor24bit(currentColor, targetColor, factor);
        int correctedPixel = (CAL_LED_COUNT + i + offset) % CAL_LED_COUNT;
        strip->setPixelColor(correctedPixel, newColor);
        currentStrip[i] = newColor;

        totalCurrent += calcEstimatedLEDCurrent(newColor, this->brightness);
    }

    // Check if totalCurrent reaches CURRENTLIMIT -> if yes reduce brightness
    if(totalCurrent > this->currentLimit){
        uint8_t newBrightness = this->brightness * float(this->currentLimit)/float(totalCurrent);
        strip->setBrightness(newBrightness);
    }
    else {
        strip->setBrightness(this->brightness);
    }

    // show strip
    strip->show();
}

/**
 * @brief Calc estimated current (mA) for one pixel with the given color and brightness
 * 
 * @param color 24bit color value of the pixel for which the current should be calculated
 * @param brightness brightness value (0-255)
 * @return the current in mA
 */
uint16_t LEDCalendar::calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness){
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
