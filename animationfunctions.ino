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
    logger.logString("Init Spiral with empty=" + String(empty));
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
    //logger.logString("x: " + String(x) + ", y: " + String(y) + "c: " + String(color) + "\n");
    counter1++;
    countStep++;
  }
  return 0;
}

/**
 * @brief Run random snake animation
 * 
 * @param init marks if call is the initial step of the animation
 * @param len length of the snake
 * @param color color of the snake
 * @param numSteps number of animation steps
 * @return int - 1 when animation is finished, else 0
 */
int randomsnake(bool init, const uint8_t len, const uint32_t color, int numSteps){
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
      if( (dir1 == down && snake1[1][len-1] >= HEIGHT-1) || 
          (dir1 == up && snake1[1][len-1] <= 0) ||
          (dir1 == right && snake1[0][len-1] >= WIDTH-1) ||
          (dir1 == left && snake1[0][len-1] <= 0)){
          dir1 = nextDir(dir1, e);  
      }
      // Random branching at the side edges
      else if((dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] >= WIDTH-1) || (dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] <= 0)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == down && snake1[1][len-1] == randomy && snake1[0][len-1] >= WIDTH-1) || (dir1 == up && snake1[1][len-1] == randomy && snake1[0][len-1] <= 0)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }
      else if((dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] <= 0) || (dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] >= HEIGHT-1)){
        dir1 = nextDir(dir1, LEFT);
        e = (e+2)%2+1;
      }
      else if((dir1 == right && snake1[0][len-1] == randomx && snake1[1][len-1] <= 0) || (dir1 == left && snake1[0][len-1] == randomx && snake1[1][len-1] >= HEIGHT-1)){
        dir1 = nextDir(dir1, RIGHT);
        e = (e+2)%2+1;
      }

      
      for(int i = 0; i < len; i++){
        // draw the snake
        ledmatrix.gridAddPixel(snake1[0][i], snake1[1][i], color);
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

/**
 * @brief Calc the next direction for led movement (snake and spiral)
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

/**
 * @brief Run random tetris animation
 * 
 * @param init marks if call is the initial step of the animation
 * @return int - 1 when animation is finished, else 0
 */
int randomtetris(bool init){
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
  static uint8_t screen[HEIGHT+3][WIDTH]; 
  // current number of blocks on the screen
  static int counterID;
  // indicate if the game was lost
  static bool gameover = false;
  
  
  if(init || gameover){
    logger.logString("Init Tetris: init=" + String(init) + ", gameover=" +  String(gameover));
    // clear local game screen
    for(int h = 0; h < HEIGHT+3; h++){
      for(int w = 0; w < WIDTH; w++){
        screen[h][w] = 0;
      }
    }
    counterID = 0;
    gameover = false;
  }
  else{
    ledmatrix.gridFlush();
    
    // list of all blocks in game, indicating which are moving
    // set every block on the screen as a potentially mover
    bool tomove[numBlocks+1];
    for(int i = 0; i < numBlocks; i++) tomove[i+1] =  i < counterID;
    
    // identify tiles which can move down (no collision below)
    for(int c = 0; c < WIDTH; c++){ // columns
      for(int r = 0; r < HEIGHT+3; r++){ // rows
        // only check pixels which are occupied
        if(screen[r][c] != 0){
          // every tile which has a pixel in last row -> no mover
          if(r == HEIGHT+2){
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
    
    if(noMoreMover){
      // no more moving blocks -> check if game over or spawn new block
      logger.logString("Tetris: No more Mover");
      gameover = false;
      // check if game was lost -> one pixel active in 4rd row (top row on the led grid)
      for(int s = 0; s < WIDTH; s++){
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
      uint8_t randx = random(0,WIDTH - 3);
      // copy shape to screen (c1 - column of block, c2 - column of screen)
      // write the id of block on the screen
      for(int c1 = 0, c2 = randx; c1 < 3; c1++, c2++){
        for(int r = 0; r < 3; r++){
          if(blockshapes[randShape][r][c1]) screen[r][c2] = counterID;
        }
      }
    }
    else{
      uint8_t moveX = WIDTH-1;
      uint8_t moveY = HEIGHT+2;
      // moving blocks exists -> move them one pixel down
      // loop over pixels and move every pixel down, which belongs to a moving block
      for(int c = WIDTH-1; c >= 0; c--){
        for(int r = HEIGHT+1; r >= 0; r--){
          if((screen[r][c] != 0) && tomove[screen[r][c]]){
            screen[r+1][c] = screen[r][c];
            screen[r][c] = 0;
            // save top left corner of block
            if(moveX > c) moveX = c;
            if(moveY > r) moveY = r;
          }
        }
      }
    }    

    // draw/copy screen values to led grid (r - row, c - column)
    for(int c = 0; c < WIDTH; c++){
      for(int r = 0; r < HEIGHT; r++){
        if(screen[r+3][c] != 0){
          // screen is 3 pixels higher than led grid, so drop the upper three lines
          ledmatrix.gridAddPixel(c,r,colors24bit[(screen[r+3][c] % NUM_COLORS)]);
        }
      }
    }
    return 0; 
  }
  return 0; 
}

