#include "snake.h"

Snake::Snake(){

}

Snake::Snake(LEDMatrix *myledmatrix, UDPLogger *mylogger){
    logger = *mylogger;
    ledmatrix = myledmatrix;
    gameState = GAME_STATE_END;
}

void Snake::loopCycle()
{
  switch(gameState)
  {
    case GAME_STATE_INIT:
      initGame();
      break;
    case GAME_STATE_RUNNING:
      updateGame();
      break;
    case GAME_STATE_END:
      break;
  }
}

void Snake::ctrlUp(){
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameState == GAME_STATE_RUNNING) {
        logger.logString("Snake: UP");
        userDirection = DIRECTION_DOWN; // need to swap direction as field is rotated 180deg
        lastButtonClick = millis();
    }
}

void Snake::ctrlDown(){
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameState == GAME_STATE_RUNNING) {
        logger.logString("Snake: DOWN");
        userDirection = DIRECTION_UP; // need to swap direction as field is rotated 180deg
        lastButtonClick = millis();
    }
}

void Snake::ctrlRight(){
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameState == GAME_STATE_RUNNING) {
        logger.logString("Snake: RIGHT");
        userDirection = DIRECTION_LEFT; // need to swap direction as field is rotated 180deg
        lastButtonClick = millis();
    }
}

void Snake::ctrlLeft(){
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameState == GAME_STATE_RUNNING) {
        logger.logString("Snake: LEFT");
        userDirection = DIRECTION_RIGHT; // need to swap direction as field is rotated 180deg
        lastButtonClick = millis();
    }
}

/**
 * @brief Clear the led matrix (turn all leds off)
 * 
 */
void Snake::resetLEDs()
{
    (*ledmatrix).gridFlush();
    (*ledmatrix).drawOnMatrixInstant();
}

void Snake::initGame()
{
    logger.logString("Snake: init");
    resetLEDs();
    head.x = 0;
    head.y = 0;
    food.x = -1;
    food.y = -1;
    wormLength = MIN_TAIL_LENGTH;
    userDirection = DIRECTION_LEFT;
    lastButtonClick = millis();

    for(int i=0; i<MAX_TAIL_LENGTH; i++) {
        tail[i].x = -1;
        tail[i].y = -1;
    }
    updateFood();
    gameState = GAME_STATE_RUNNING;
}

void Snake::updateGame()
{
  if ((millis() - lastDrawUpdate) > GAME_DELAY) {
    logger.logString("Snake: update game");
    toggleLed(tail[wormLength-1].x, tail[wormLength-1].y, LED_TYPE_OFF);
    switch(userDirection) {
      case DIRECTION_RIGHT:
        if (head.x > 0) {
          head.x--;
        }
        break;
      case DIRECTION_LEFT:
        if (head.x < X_MAX-1) {
          head.x++;
        }
        break;
      case DIRECTION_DOWN:
        if (head.y > 0) {
          head.y--;
        }
        break;
      case DIRECTION_UP:
        if (head.y < Y_MAX-1) {
          head.y++;
        }
        break;
    }

    if (isCollision() == true) {
      endGame();
      return;
    }

    updateTail();

    if (head.x == food.x && head.y == food.y) {
      if (wormLength < MAX_TAIL_LENGTH) {
        wormLength++;
      }
      updateFood();
    }

    lastDrawUpdate = millis();
    (*ledmatrix).drawOnMatrixInstant();
  }
}

void Snake::endGame()
{
  gameState = GAME_STATE_END;
  toggleLed(head.x, head.y, LED_TYPE_BLOOD);
  (*ledmatrix).drawOnMatrixInstant();
}

void Snake::updateTail()
{
  for(int i=wormLength-1; i>0; i--) {
    tail[i].x = tail[i-1].x;
    tail[i].y = tail[i-1].y;
  }
  tail[0].x = head.x;
  tail[0].y = head.y;

  for(int i=0; i<wormLength; i++) {
    if (tail[i].x > -1) {
      toggleLed(tail[i].x, tail[i].y, LED_TYPE_SNAKE);
    }
  }
}

void Snake::updateFood()
{
  bool found = true;
  do {
    found = true;
    food.x = random(0, X_MAX);
    food.y = random(0, Y_MAX);
    for(int i=0; i<wormLength; i++) {
      if (tail[i].x == food.x && tail[i].y == food.y) {
         found = false;
      }
    }
  } while(found == false);
  toggleLed(food.x, food.y, LED_TYPE_FOOD);
}

bool Snake::isCollision()
{
  if (head.x < 0 || head.x >= X_MAX) {
    return true;
  }
  if (head.y < 0 || head.y >= Y_MAX) {
    return true;
  }
  for(int i=1; i<wormLength; i++) {
    if (tail[i].x == head.x && tail[i].y == head.y) {
       return true;
    }
  }
  return false;
}

void Snake::toggleLed(uint8_t x, uint8_t y, uint8_t type)
{
  uint32_t color;

  switch(type) {
    case LED_TYPE_SNAKE:
      color = LEDMatrix::Color24bit(0, 100, 100);
      break;
    case LED_TYPE_OFF:
      color = LEDMatrix::Color24bit(0, 0, 0);
      break;
    case LED_TYPE_FOOD:
      color = LEDMatrix::Color24bit(0, 150, 0);
      break;
    case LED_TYPE_BLOOD:
      color = LEDMatrix::Color24bit(150, 0, 0);
      break;
  }

  (*ledmatrix).gridAddPixel(x, y, color);
}