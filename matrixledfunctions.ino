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

//! Interpolates two colors and returns an color of the result
/*!
 * \param color1 startcolor for interpolation
 * \param color2 endcolor for interpolation
 * \param factor which color is wanted on the path from start to end color
 * \returns interpolated color
 */
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

// "activates" a pixel in grid
void gridAddPixel(uint8_t x,uint8_t y){
    grid[y][x] = 1;
}

// "deactivates" all pixels in grid
void gridFlush(void){
    //Setzt an jeder Position eine 0
    for(uint8_t i=0; i<height; i++){
        for(uint8_t j=0; j<width; j++){
            grid[i][j] = 0;
        }
    }
}


// draws the grid to the ledmatrix with the current active color
void drawOnMatrix(uint32_t color){
  for(int s = 0; s < width; s++){
    for(int z = 0; z < height; z++){
      if(grid[z][s] != 0){
        Serial.print("1 ");
        matrix.drawPixel(s, z, color); 
      }
      else{
        Serial.print("0 ");
      }
    }
    Serial.println();  
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
    //logger.logString("x: " + String(x) + ", y: " + String(y) + "\n");
    counter1++;
    countStep++;
  }
  return 0;
}


int snake(bool init, const uint8_t len, const uint16_t color){
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
  if (countStep == 200){
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
      if( (dir1 == down && snake1[1][len-1] == height-1) || 
          (dir1 == up && snake1[1][len-1] == 0) ||
          (dir1 == right && snake1[0][len-1] == width-1) ||
          (dir1 == left && snake1[0][len-1] == 0)){
          dir1 = nextDir(dir1, e);  
      }
      // Random branching at the side edges
      else if((dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] == width-1) || (dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] == 0)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] == width-1) || (dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] == 0)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }
      else if((dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] == 0) || (dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] == height-1)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] == 0) || (dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] == height-1)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }

      
      for(int i = 0; i < len; i++){
        // draw the snake
        matrix.drawPixel(snake1[0][i], snake1[1][i], color);
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

