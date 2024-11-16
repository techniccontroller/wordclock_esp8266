#include "ledmatrix.h"
#include "own_font.h"

/**
 * @brief Construct a new LEDMatrix::LEDMatrix object
 * 
 * @param mymatrix pointer to Adafruit_NeoMatrix object
 * @param mybrightness the initial brightness of the leds
 * @param mylogger pointer to the UDPLogger object
 */
LEDMatrix::LEDMatrix(Adafruit_NeoMatrix *mymatrix, uint8_t mybrightness, UDPLogger *mylogger){
    neomatrix = mymatrix;
    brightness = mybrightness;
    logger = mylogger;
    currentLimit = DEFAULT_CURRENT_LIMIT;
}

/**
 * @brief Convert RGB value to 24bit color value
 * 
 * @param r red value (0-255)
 * @param g green value (0-255)
 * @param b blue value (0-255)
 * @return uint32_t 24bit color value
 */
uint32_t LEDMatrix::Color24bit(uint8_t r, uint8_t g, uint8_t b) 
{
  return ((uint32_t)r << 16) | ((uint32_t)g <<  8) | b;
}

/**
 * @brief Convert 24bit color to 16bit color
 * 
 * @param color24bit 24bit color value
 * @return uint16_t 16bit color value
 */
uint16_t LEDMatrix::color24to16bit(uint32_t color24bit){
  uint8_t r = color24bit >> 16 & 0xff;
  uint8_t g = color24bit >> 8 & 0xff;
  uint8_t b = color24bit & 0xff;
  return ((uint16_t)(r & 0xF8) << 8) |
         ((uint16_t)(g & 0xFC) << 3) |
                    (b         >> 3);
}

/**
 * @brief Input a value 0 to 255 to get a color value. The colors are a transition r - g - b - back to r.
 * 
 * @param WheelPos Value between 0 and 255
 * @return uint32_t return 24bit color of colorwheel
 */
uint32_t LEDMatrix::Wheel(uint8_t WheelPos)
{
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
 * @param color2 endcolor for interpolatio
 * @param factor which color is wanted on the path from start to end color
 * @return uint32_t interpolated color
 */
uint32_t LEDMatrix::interpolateColor24bit(uint32_t color1, uint32_t color2, float factor)
{
    uint8_t resultRed = color1 >> 16 & 0xff;
    uint8_t resultGreen = color1 >> 8 & 0xff;
    uint8_t resultBlue = color1 & 0xff;
    resultRed = (uint8_t)(resultRed + (int16_t)(factor * ((int16_t)(color2 >> 16 & 0xff) - (int16_t)resultRed)));
    resultGreen = (uint8_t)(resultGreen + (int16_t)(factor * ((int16_t)(color2 >> 8 & 0xff) - (int16_t)resultGreen)));
    resultBlue = (uint8_t)(resultBlue + (int16_t)(factor * ((int16_t)(color2 & 0xff) - (int16_t)resultBlue)));
    return Color24bit(resultRed, resultGreen, resultBlue);
}

/**
 * @brief Setup function for LED matrix
 * 
 */
void LEDMatrix::setupMatrix() 
{
    (*neomatrix).begin();       
    (*neomatrix).setTextWrap(false);
    (*neomatrix).setBrightness(brightness);
    randomSeed(analogRead(0));
}

/**
 * @brief Turn on the minutes indicator leds with the provided pattern (binary encoded)
 * 
 * @param pattern the binary encoded pattern of the minute indicator
 * @param color color to be displayed
 */
void LEDMatrix::setMinIndicator(uint8_t pattern, uint32_t color)
{
  // pattern:
  // 15 -> 1111
  // 14 -> 1110
  // (...)
  //  2 -> 0010
  //  1 -> 0001
  //  0 -> 0000
  if(pattern & 1){
    targetindicators[0] = color;
  }
  if(pattern >> 1 & 1){
    targetindicators[1] = color;
  }
  if(pattern >> 2 & 1){
    targetindicators[2] = color;
  }
  if(pattern >> 3 & 1){
    targetindicators[3] = color;
  }
}

/**
 * @brief "Activates" a pixel in targetgrid with color
 * 
 * @param x x-position of pixel
 * @param y y-position of pixel
 * @param color color of pixel
 */
void LEDMatrix::gridAddPixel(uint8_t x, uint8_t y, uint32_t color)
{
  // limit ranges of x and y
  if(x < WIDTH && y < HEIGHT){
    targetgrid[y][x] = color;
  }
  else{
    //logger->logString("Index out of Range: " + String(x) + ", " + String(y));
  }
}

/**
 * @brief "Deactivates" all pixels in targetgrid
 * 
 */
void LEDMatrix::gridFlush(void)
{
    // set a zero to each pixel
    for(uint8_t i=0; i<HEIGHT; i++){
        for(uint8_t j=0; j<WIDTH; j++){
            targetgrid[i][j] = 0;
        }
    }
    // set every minutes indicator led to 0
    targetindicators[0] = 0;
    targetindicators[1] = 0;
    targetindicators[2] = 0;
    targetindicators[3] = 0;
}

/**
 * @brief Write target pixels directly to leds
 * 
 */
void LEDMatrix::drawOnMatrixInstant(){
  drawOnMatrix(1.0);
}

/**
 * @brief Write target pixels with low pass filter to leds
 * 
 * @param factor factor between 0 and 1 (1.0 = hard, 0.1 = smooth)
 */
void LEDMatrix::drawOnMatrixSmooth(float factor){
  drawOnMatrix(factor);
}

/**
 * @brief Draws the targetgrid to the ledmatrix
 * 
 * @param factor factor between 0 and 1 (1.0 = hard, 0.1 = smooth)
 */
void LEDMatrix::drawOnMatrix(float factor){
  uint16_t totalCurrent = 0;
  // loop over all leds in matrix
  for(int s = 0; s < WIDTH; s++){
    for(int z = 0; z < HEIGHT; z++){
      // inplement momentum as smooth transistion function
      uint32_t filteredColor = interpolateColor24bit(currentgrid[z][s], targetgrid[z][s], factor);
      (*neomatrix).drawPixel(s, z, color24to16bit(filteredColor)); 
      currentgrid[z][s] = filteredColor;
      totalCurrent += calcEstimatedLEDCurrent(filteredColor);
    } 
  }

  // loop over all minute indicator leds
  for(int i = 0; i < 4; i++){
    uint32_t filteredColor = interpolateColor24bit(currentindicators[i], targetindicators[i], factor);
    (*neomatrix).drawPixel(WIDTH - (1+i), HEIGHT, color24to16bit(filteredColor));
    currentindicators[i] = filteredColor;
    totalCurrent += calcEstimatedLEDCurrent(filteredColor);
  }

  // Check if totalCurrent reaches CURRENTLIMIT -> if yes reduce brightness
  if(totalCurrent > currentLimit){
    uint8_t newBrightness = brightness * float(currentLimit)/float(totalCurrent);
    //logger->logString("CurrentLimit reached!!!: " + String(totalCurrent) + ", new: " + String(newBrightness));
    (*neomatrix).setBrightness(newBrightness);
  }
  (*neomatrix).show();
}

/**
 * @brief Shows a 1-digit number on LED matrix (5x3)
 * 
 * @param xpos x of left top corner of digit
 * @param ypos y of left top corner of digit
 * @param number number to display
 * @param color color to display (24bit)
 */
void LEDMatrix::printNumber(uint8_t xpos, uint8_t ypos, uint8_t number, uint32_t color)
{
  for(int y=ypos, i = 0; y < (ypos+5); y++, i++){
    for(int x=xpos, k = 2; x < (xpos+3); x++, k--){
      if((numbers_font[number][i] >> k) & 0x1){
        gridAddPixel(x, y, color);
      }
    }
  }
}

/**
 * @brief Shows a character on LED matrix (5x3), supports currently only 'I' and 'P'
 * 
 * @param xpos x of left top corner of character
 * @param ypos y of left top corner of character
 * @param character character to display
 * @param color color to display (24bit)
 */
void LEDMatrix::printChar(uint8_t xpos, uint8_t ypos, char character, uint32_t color)
{
  int id = 0;
  if(character == 'I'){
    id = 0;
  }
  else if(character == 'P'){
    id = 1;
  }

  for(int y=ypos, i = 0; y < (ypos+5); y++, i++){
    for(int x=xpos, k = 2; x < (xpos+3); x++, k--){
      if((chars_font[id][i] >> k) & 0x1){
        gridAddPixel(x, y, color);
      }
    }
  }
}

/**
 * @brief Set Brightness
 * 
 * @param mybrightness brightness to be set [0..255]
 */
void LEDMatrix::setBrightness(uint8_t mybrightness){
  brightness = mybrightness;
  (*neomatrix).setBrightness(brightness);
}

/**
 * @brief Calc estimated current (mA) for one pixel with the given color and brightness
 * 
 * @param color 24bit color value of the pixel for which the current should be calculated
 * @return the current in mA
 */
uint16_t LEDMatrix::calcEstimatedLEDCurrent(uint32_t color){
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

/**
 * @brief Set the current limit
 * 
 * @param mycurrentLimit the total current limit for whole matrix
 */
void LEDMatrix::setCurrentLimit(uint16_t mycurrentLimit){
  currentLimit = mycurrentLimit;
}