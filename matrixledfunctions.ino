const int8_t dx[] = {1, -1, 0, 0};
const int8_t dy[] = {0, 0, -1, 1};

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return matrix.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return matrix.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return matrix.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// Interpolates two colors and returns an color of the result
uint32_t interpolateColor(uint32_t color1, uint32_t color2, float factor)
{
    uint8_t resultRed = color1 >> 16 & 0xff;
    uint8_t resultGreen = color1 >> 8 & 0xff;
    uint8_t resultBlue = color1 & 0xff;
    resultRed = (int16_t)(resultRed + (int16_t)(factor * ((int16_t)(color2 >> 16 & 0xff) - (int16_t)resultRed)));
    resultGreen = (int16_t)(resultGreen + (int16_t)(factor * ((int16_t)(color2 >> 8 & 0xff) - (int16_t)resultGreen)));
    resultBlue = (int16_t)(resultBlue + (int16_t)(factor * ((int16_t)(color2 & 0xff) - (int16_t)resultBlue)));
    return matrix.Color(resultRed, resultGreen, resultBlue);
}

//setup function LED
void setupMatrix()
{
    matrix.begin();       
    matrix.setTextWrap(false);
    matrix.setBrightness(brightness);
    matrix.setTextColor(colors[0]);
    randomSeed(analogRead(0));
}

//! turn on the minutes indicator leds with the provided pattern (binary encoded)
/*!
 * \param pattern the binary encoded pattern of the minute indicator
 * \param color color to be displayed
 */
void setMinIndicator(uint8_t pattern, uint32_t color){
  // pattern:
  // 15 -> 1111
  // 14 -> 1110
  // (...)
  //  2 -> 0010
  //  1 -> 0001
  //  0 -> 0000
  if(pattern & 1){
    matrix.drawPixel(width - 1, height, color);
  }
  if(pattern >> 1 & 1){
    matrix.drawPixel(width - 2, height, color);
  }
  if(pattern >> 2 & 1){
    matrix.drawPixel(width - 3, height, color);
  }
  if(pattern >> 3 & 1){
    matrix.drawPixel(width - 4, height, color);
  }
  matrix.show();
}

//! Function to draw a spiral step (from center)
/*! 
 * \param init marks if call is the initial step of the spiral
 * \param empty marks if the spiral should 'draw' empty leds
 * \param size the size of the spiral in leds
 * \return 1 if end is reached, else 0
 */
int spiral(bool init, bool empty, uint8_t size){
  static direction dir1;   // current direction
  static int x;
  static int y;
  static int counter1;
  static int countStep;
  static int countEdge;
  static int countCorner;
  static bool breiter ;
  static int randNum;
  if(init){
    dir1 = down;          // current direction
    x = width/2;
    y = width/2;
    if(!empty)matrix.fillScreen(0);
    counter1 = 0;
    countStep = 0;
    countEdge = 1;
    countCorner = 0;
    breiter = true;
    randNum = random(255);
  }

  if (countStep == size*size){
    // End reached return 1
    return 1;
  }
  else{
    
    if(empty){
      matrix.drawPixel(x, y, 0);
    }
    else{
      matrix.drawPixel(x, y, Wheel((randNum +countStep*6)%255));
    }
    Serial.print(counter1);
    Serial.print(countEdge);
    Serial.println(countCorner);
    if(countCorner == 2 && breiter){
      countEdge +=1;
      breiter = false;
    }
    if(counter1 >= countEdge){
      dir1 = nextDir(dir1, LEFT);
      counter1 = 0;
      countCorner++;
    }
    if(countCorner >= 4){
      countCorner = 0;
      countEdge += 1;
      breiter = true;
    }
    
    x += dx[dir1];
    y += dy[dir1];
    logger.logString("x: " + String(x) + ", y: " + String(y) + "\n");
    counter1++;
    countStep++;
  }
  return 0;
}


//! calc the next direction for led movement (snake and spiral)
/*!
 * \param dir direction of the current led movement
 * \param d action to be executed
 * \return next direction
 */
direction nextDir(direction dir, int d){
  // d = 0 -> continue straight on
  // d = 1 -> turn LEFT
  // d = 2 -> turn RIGHT
  direction selection[3];
  switch(dir){
    case right: 
      selection[0] = right;
      selection[1] = up;
      selection[2] = down;
      break;
    case left:
      selection[0] = left;
      selection[1] = down;
      selection[2] = up;
      break;
    case up:
      selection[0] = up;
      selection[1] = left;
      selection[2] = right;
      break;
    case down:
      selection[0] = down;
      selection[1] = right;
      selection[2] = left;
      break; 
  }
  direction next = selection[d];
  return next;
}

