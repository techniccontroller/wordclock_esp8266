const int8_t dx[] = {1, -1, 0, 0};
const int8_t dy[] = {0, 0, -1, 1};

uint16_t color24to16bit(uint32_t color24bit){
  uint8_t r = color24bit >> 16 & 0xff;
  uint8_t g = color24bit >> 8 & 0xff;
  uint8_t b = color24bit & 0xff;
  return ((uint16_t)(r & 0xF8) << 8) |
         ((uint16_t)(g & 0xFC) << 3) |
                    (b         >> 3);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(uint8_t WheelPos)
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

//! Interpolates two colors24bit and returns an color of the result
/*!
 * \param color1 startcolor for interpolation
 * \param color2 endcolor for interpolation
 * \param factor which color is wanted on the path from start to end color
 * \returns interpolated color
 */
uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor)
{
    
    uint8_t resultRed = color1 >> 16 & 0xff;
    uint8_t resultGreen = color1 >> 8 & 0xff;
    uint8_t resultBlue = color1 & 0xff;
    resultRed = (uint8_t)(resultRed + (int16_t)(factor * ((int16_t)(color2 >> 16 & 0xff) - (int16_t)resultRed)));
    resultGreen = (uint8_t)(resultGreen + (int16_t)(factor * ((int16_t)(color2 >> 8 & 0xff) - (int16_t)resultGreen)));
    resultBlue = (uint8_t)(resultBlue + (int16_t)(factor * ((int16_t)(color2 & 0xff) - (int16_t)resultBlue)));
    return Color24bit(resultRed, resultGreen, resultBlue);
}

//setup function LED
void setupMatrix()
{
    matrix.begin();       
    matrix.setTextWrap(false);
    matrix.setBrightness(brightness);
    matrix.setTextColor(colors24bit[0]);
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
    targetindicators[0] = color24to16bit(color);
  }
  if(pattern >> 1 & 1){
    targetindicators[1] = color24to16bit(color);
  }
  if(pattern >> 2 & 1){
    targetindicators[2] = color24to16bit(color);
  }
  if(pattern >> 3 & 1){
    targetindicators[3] = color24to16bit(color);
  }
}

// "activates" a pixel in targetgrid with color
void gridAddPixel(uint8_t x, uint8_t y, uint32_t color){
  // limit ranges of x and y
  if(x >= 0 && x < width && y >= 0 && y < height){
    targetgrid[y][x] = color;
  }
  else{
    logger.logString("Index out of Range: " + String(x) + ", " + String(y));
  }
}

// "deactivates" all pixels in targetgrid
void gridFlush(void){
    // set a zero to each pixel
    for(uint8_t i=0; i<height; i++){
        for(uint8_t j=0; j<width; j++){
            targetgrid[i][j] = 0;
        }
    }
    // set every minutes indicator led to 0
    targetindicators[0] = 0;
    targetindicators[1] = 0;
    targetindicators[2] = 0;
    targetindicators[3] = 0;
}

void drawOnMatrixInstant(){
  drawOnMatrix(1.0);
}

void drawOnMatrixSmooth(float factor){
  drawOnMatrix(factor);
}


// draws the targetgrid to the ledmatrix with the current active color
void drawOnMatrix(float factor){
  for(int s = 0; s < width; s++){
    for(int z = 0; z < height; z++){
      // inplement momentum as smooth transistion function
      uint32_t filteredColor = interpolateColor24bit(currentgrid[z][s], targetgrid[z][s], factor);
      matrix.drawPixel(s, z, color24to16bit(filteredColor)); 
      currentgrid[z][s] = filteredColor;
    } 
  }

  for(int i = 0; i < 4; i++){
    uint32_t filteredColor = interpolateColor24bit(currentindicators[i], targetindicators[i], factor);
    matrix.drawPixel(width - (1+i), height, color24to16bit(filteredColor));
    currentindicators[i] = filteredColor;
  }
}

//! Function to draw a spiral step (from center)
/*! 
 * \param init marks if call is the initial step of the spiral
 * \param empty marks if the spiral should 'draw' empty leds
 * \param size the size of the spiral in leds
 * \returns 1 if end is reached, else 0
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
    logger.logString("Init Spiral with empty=" + String(empty));
    dir1 = down;          // current direction
    x = width/2;
    y = width/2;
    if(!empty)gridFlush();
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
    uint32_t color = Wheel((randNum +countStep*6)%255);
    // if draw mode is empty, set color to zero
    if(empty){
      color = 0;
    }
    gridAddPixel(x, y, color);
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
    //logger.logString("x: " + String(x) + ", y: " + String(y) + "c: " + String(color) + "\n");
    counter1++;
    countStep++;
  }
  return 0;
}


int snake(bool init, const uint8_t len, const uint32_t color, int numSteps){
  static direction dir1;
  static int snake1[2][10];
  static int randomy;
  static int randomx;
  static int e;
  static int countStep;
  if(init){
    dir1 = down;        // current direction
    for(int i = 0; i < len; i++){
      snake1[0][i] = 3;
      snake1[1][i] = i;
    }
    
    randomy = random(1,8);    // Random variable for y-direction
    randomx = random(1,4);    // Random variable for x-direction
    e = LEFT;                 // next turn
    countStep = 0;
  }
  if (countStep == numSteps){
    // End reached return 1
    return 1;
  }
  else{ 
      // move one step forward
      for(int i = 0; i < len; i++){
        if(i < len-1){
          snake1[0][i] = snake1[0][i+1];
          snake1[1][i] = snake1[1][i+1];
        }else{
          snake1[0][i] = snake1[0][i]+dx[dir1];
          snake1[1][i] = snake1[1][i]+dy[dir1];
        }
      }
      // collision with wall?
      if( (dir1 == down && snake1[1][len-1] >= height-1) || 
          (dir1 == up && snake1[1][len-1] <= 0) ||
          (dir1 == right && snake1[0][len-1] >= width-1) ||
          (dir1 == left && snake1[0][len-1] <= 0)){
          dir1 = nextDir(dir1, e);  
      }
      // Random branching at the side edges
      else if((dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] >= width-1) || (dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] <= 0)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] >= width-1) || (dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] <= 0)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }
      else if((dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] <= 0) || (dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] >= height-1)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] <= 0) || (dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] >= height-1)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }

      
      for(int i = 0; i < len; i++){
        // draw the snake
        gridAddPixel(snake1[0][i], snake1[1][i], color);
      }

      // calc new random variables after every 20 steps
      if(countStep%20== 0){
        randomy = random(1,8);
        randomx = random(1,4);
      }
      countStep++;
  }
  return 0;
}

//! calc the next direction for led movement (snake and spiral)
/*!
 * \param dir direction of the current led movement
 * \param d action to be executed
 * \returns next direction
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
 * @brief show the time as digits on the wordclock
 * 
 * @param hours hours of time to display
 * @param minutes minutes of time to display
 * @param color  color to display (24bit)
 */
void showDigitalClock(uint8_t hours, uint8_t minutes, uint32_t color){
  uint8_t fstDigitH = hours/10;
  uint8_t sndDigitH = hours%10;
  uint8_t fstDigitM = minutes/10;
  uint8_t sndDigitM = minutes%10;
  printNumber(1, 0, fstDigitH, color);
  printNumber(5, 0, sndDigitH, color);
  printNumber(1, 6, fstDigitM, color);
  printNumber(5, 6, sndDigitM, color);
}

/**
 * @brief show a 1-digit number on LED matrix (5x3)
 * 
 * @param xpos x of left top corner of digit
 * @param ypos y of left top corner of digit
 * @param number number to display
 * @param color color to display (24bit)
 */
void printNumber(uint8_t xpos, uint8_t ypos, uint8_t number, uint32_t color){
  for(int y=ypos, i = 0; y < (ypos+5); y++, i++){
    for(int x=xpos, k = 2; x < (xpos+3); x++, k--){
      if((numbers_font[number][i] >> k) & 0x1){
        Serial.print(1);
        gridAddPixel(x, y, color);
      }
    }
  }
}

/**
 * @brief show a character on LED matrix (5x3), supports currently only 'I' and 'P'
 * 
 * @param xpos x of left top corner of character
 * @param ypos y of left top corner of character
 * @param character character to display
 * @param color color to display (24bit)
 */
void printChar(uint8_t xpos, uint8_t ypos, char character, uint32_t color){
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
        Serial.print(1);
        gridAddPixel(x, y, color);
      }
    }
  }
}


int tetris(bool init){
  // total number of blocks which can be displayed
  const static uint8_t numBlocks = 30;
  // all different block shapes
  const static bool blockshapes[9][3][3]={{ {0,0,0},
                                            {0,0,0},
                                            {0,0,0}},
                                          { {1,0,0},
                                            {1,0,0},
                                            {1,0,0}},
                                          { {0,0,0},
                                            {1,0,0},
                                            {1,0,0}},
                                          { {0,0,0},
                                            {1,1,0},
                                            {1,0,0}},
                                          { {0,0,0},
                                            {0,0,0},
                                            {1,1,0}},
                                          { {0,0,0},
                                            {1,1,0},
                                            {1,1,0}},
                                          { {0,0,0},
                                            {0,0,0},
                                            {1,1,1}},
                                          { {0,0,0},
                                            {1,1,1},
                                            {1,0,0}},
                                          { {0,0,0},
                                            {0,0,1},
                                            {1,1,1}}};
  // local game screen buffer
  static uint8_t screen[height+3][width]; 
  // current number of blocks on the screen
  static int counterID;
  // indicate if the game was lost
  static bool gameover = false;
  
  
  if(init || gameover){
    logger.logString("Init Tetris: init=" + String(init) + ", gameover=" +  String(gameover));
    // clear local game screen
    for(int h = 0; h < height+3; h++){
      for(int w = 0; w < width; w++){
        screen[h][w] = 0;
      }
    }
    counterID = 0;
    gameover = false;
  }
  else{
    gridFlush();
    
    // list of all blocks in game, indicating which are moving
    // set every block on the screen as a potentially mover
    bool tomove[numBlocks+1];
    for(int i = 0; i < numBlocks; i++) tomove[i+1] =  i < counterID;
    
    // identify tiles which can move down (no collision below)
    for(int c = 0; c < width; c++){ // columns
      for(int r = 0; r < height+3; r++){ // rows
        // only check pixels which are occupied
        if(screen[r][c] != 0){
          // every tile which has a pixel in last row -> no mover
          if(r == height+2){
            tomove[screen[r][c]] = false;
          }
          // or every pixel
          else if(screen[r+1][c] != 0 && screen[r+1][c] != screen[r][c]){
            tomove[screen[r][c]] = false;
          }
        }
      }  
    }

    // indicate if there is no moving block
    // assume first there are no more moving block
    bool noMoreMover = true;
    // loop over existing block and ask if they can move
    for(int i = 0; i < counterID; i++){
      if(tomove[i+1]){
        noMoreMover = false;
      }
    }

    String test = "";
    for(int i = 0; i < numBlocks; i++) test+=String(tomove[i+1]);
    logger.logString(test);
    
    if(noMoreMover){
      // no more moving blocks -> check if game over or spawn new block
      logger.logString("Tetris: No more Mover");
      gameover = false;
      // check if game was lost -> one pixel active in 4rd row (top row on the led grid)
      for(int s = 0; s < width; s++){
        if(screen[3][s] != 0) gameover = true;
      }
      if(gameover || counterID >= (numBlocks-1)){
        logger.logString("Tetris: Gameover");
        return 1;
      }

      // Create new block
      // increment counter 
      counterID++;
      // select random shape for new block
      uint8_t randShape = random(1,9);
      // select random position (column) for spawn of new block
      uint8_t randx = random(0,width - 3);
      // copy shape to screen (c1 - column of block, c2 - column of screen)
      // write the id of block on the screen
      logger.logString("newBlock: " + String(randShape) + ", " + String(randx));
      for(int c1 = 0, c2 = randx; c1 < 3; c1++, c2++){
        for(int r = 0; r < 3; r++){
          if(blockshapes[randShape][r][c1]) screen[r][c2] = counterID;
        }
      }


      for(int r = 0; r < (height+3); r++){ // rows
        String row = "";
        for(int c = 0; c < width; c++){ // columns
          row += String(screen[r][c]) + ",";
        }
        logger.logString(row);
        delay(5);
      }
      
      logger.logString("Tetris: num of blocks = " + String(counterID));
    }
    else{
      uint8_t tempscreen[height+3][width] = {0};
      uint8_t moveX = width-1;
      uint8_t moveY = height+2;
      // moving blocks exists -> move them one pixel down
      // loop over pixels and move every pixel down, which belongs to a moving block
      for(int c = width-1; c >= 0; c--){
        for(int r = height+1; r >= 0; r--){
          if((screen[r][c] != 0) && tomove[screen[r][c]]){
            tempscreen[r+1][c] = screen[r][c];
            screen[r+1][c] = screen[r][c];
            screen[r][c] = 0;
            // save top left corner of block
            if(moveX > c) moveX = c;
            if(moveY > r) moveY = r;
          }
        }
      }

      /*logger.logString("rotation point: " + String(moveX) + ", " + String(moveY));
      delay(5);
      for(int r = 0; r < (height+3); r++){ // rows
        String row = "";
        for(int c = 0; c < width; c++){ // columns
          
          row += String(tempscreen[r][c]) + ",";
        }
        logger.logString(row);
        delay(5);
      }*/
    }    

    // draw/copy screen values to led grid (r - row, c - column)
    for(int c = 0; c < width; c++){
      for(int r = 0; r < height; r++){
        if(screen[r+3][c] != 0){
          // screen is 3 pixels higher than led grid, so drop the upper three lines
          gridAddPixel(c,r,colors24bit[(screen[r+3][c] % NUM_COLORS)]);
          //logger.logString("x: " + String(c) + ", y= " + String(r));
        }
      }
    }
    return 0; 
  }
  return 0; 
}


int rotate90X(int oldX, int oldY, bool clockwise){
  int newX = 0;
  if(clockwise){
    newX = oldY;
  }else{
    newX = -1 * oldY;
  }
  return newX;
}

int rotate90Y(int oldX, int oldY, bool clockwise){
  int newY = 0;
  if(clockwise){
    newY = -1 * oldX;
  }else{
    newY = oldX;
  }
  return newY;
}

