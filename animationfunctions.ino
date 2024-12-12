const int8_t dx[] = {1, -1, 0, 0};
const int8_t dy[] = {0, 0, -1, 1};

/**
 * @brief Function to draw a spiral step (from center)
 * 
 * @param init marks if call is the initial step of the spiral
 * @param empty marks if the spiral should 'draw' empty leds
 * @param size the size of the spiral in leds
 * @return int - 1 if end is reached, else 0
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
    Serial.println("Init Spiral with empty=" + String(empty));
    dir1 = down;          // current direction
    x = WIDTH/2;
    y = WIDTH/2;
    if(!empty)ledmatrix.gridFlush();
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
    // calc color from colorwheel
    uint32_t color = LEDMatrix::Wheel((randNum +countStep*6)%255);
    // if draw mode is empty, set color to zero
    if(empty){
      color = 0;
    }
    ledmatrix.gridAddPixel(x, y, color);
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
    //Serial.println("x: " + String(x) + ", y: " + String(y) + "c: " + String(color) + "\n");
    counter1++;
    countStep++;
  }
  return 0;
}

/**
 * @brief Calc the next direction for led movement (spiral)
 * 
 * @param dir direction of the current led movement
 * @param d action to be executed
 * @return direction - next direction
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

/**
 * @brief Show the time as digits on the wordclock
 * 
 * @param hours hours of time to display
 * @param minutes minutes of time to display
 * @param color  color to display (24bit)
 */
void showDigitalClock(uint8_t hours, uint8_t minutes, uint32_t color){
  ledmatrix.gridFlush();
  uint8_t fstDigitH = hours/10;
  uint8_t sndDigitH = hours%10;
  uint8_t fstDigitM = minutes/10;
  uint8_t sndDigitM = minutes%10;
  ledmatrix.printNumber(2, 0, fstDigitH, color);
  ledmatrix.printNumber(6, 0, sndDigitH, color);
  ledmatrix.printNumber(2, 6, fstDigitM, color);
  ledmatrix.printNumber(6, 6, sndDigitM, color);
}


