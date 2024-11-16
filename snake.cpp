/**
 * @file snake.cpp
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class implementation of snake game
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright Copyright (c) 2022
 * 
 * main code from https://elektro.turanis.de/html/prj099/index.html
 * 
 */
#include "snake.h"

/**
 * @brief Construct a new Snake:: Snake object
 * 
 */
Snake::Snake(){

}

/**
 * @brief Construct a new Snake:: Snake object
 * 
 * @param myledmatrix pointer to LEDMatrix object, need to provide gridAddPixel(x, y, col), gridFlush()
 * @param mylogger pointer to UDPLogger object, need to provide a function logString(message)
 */
Snake::Snake(LEDMatrix *myledmatrix, UDPLogger *mylogger){
    _logger = mylogger;
    _ledmatrix = myledmatrix;
    _gameState = GAME_STATE_END;
}

/**
 * @brief Run main loop for one cycle
 * 
 */
void Snake::loopCycle()
{
  switch(_gameState)
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

/**
 * @brief Trigger control: UP
 * 
 */
void Snake::ctrlUp(){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_SNAKE && _gameState == GAME_STATE_RUNNING) {
        (*_logger).logString("Snake: UP");
        _userDirection = DIRECTION_DOWN; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Trigger control: DOWN
 * 
 */
void Snake::ctrlDown(){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_SNAKE && _gameState == GAME_STATE_RUNNING) {
        (*_logger).logString("Snake: DOWN");
        _userDirection = DIRECTION_UP; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Trigger control: RIGHT
 * 
 */
void Snake::ctrlRight(){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_SNAKE && _gameState == GAME_STATE_RUNNING) {
        (*_logger).logString("Snake: RIGHT");
        _userDirection = DIRECTION_LEFT; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Trigger control: LEFT
 * 
 */
void Snake::ctrlLeft(){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_SNAKE && _gameState == GAME_STATE_RUNNING) {
        (*_logger).logString("Snake: LEFT");
        _userDirection = DIRECTION_RIGHT; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Clear the led matrix (turn all leds off)
 * 
 */
void Snake::resetLEDs()
{
    (*_ledmatrix).gridFlush();
}

/**
 * @brief Initialize a new game
 * 
 */
void Snake::initGame()
{
    (*_logger).logString("Snake: init");
    resetLEDs();
    _head.x = 0;
    _head.y = 0;
    _food.x = -1;
    _food.y = -1;
    _wormLength = MIN_TAIL_LENGTH;
    _userDirection = DIRECTION_LEFT;
    _lastButtonClick = millis();

    for(int i=0; i<MAX_TAIL_LENGTH; i++) {
        _tail[i].x = -1;
        _tail[i].y = -1;
    }
    updateFood();
    _gameState = GAME_STATE_RUNNING;
}

/**
 * @brief Update game representation
 * 
 */
void Snake::updateGame()
{
  if ((millis() - _lastDrawUpdate) > GAME_DELAY_SNAKE) {
    (*_logger).logString("Snake: update game");
    toggleLed(_tail[_wormLength-1].x, _tail[_wormLength-1].y, LED_TYPE_EMPTY);
    switch(_userDirection) {
      case DIRECTION_RIGHT:
        if (_head.x > 0) {
          _head.x--;
        }
        break;
      case DIRECTION_LEFT:
        if (_head.x < X_MAX-1) {
          _head.x++;
        }
        break;
      case DIRECTION_DOWN:
        if (_head.y > 0) {
          _head.y--;
        }
        break;
      case DIRECTION_UP:
        if (_head.y < Y_MAX-1) {
          _head.y++;
        }
        break;
    }

    if (isCollision() == true) {
      endGame();
      return;
    }

    updateTail();

    if (_head.x == _food.x && _head.y == _food.y) {
      if (_wormLength < MAX_TAIL_LENGTH) {
        _wormLength++;
      }
      updateFood();
    }

    _lastDrawUpdate = millis();
  }
}

/**
 * @brief Game over, draw _head red
 * 
 */
void Snake::endGame()
{
  _gameState = GAME_STATE_END;
  toggleLed(_head.x, _head.y, LED_TYPE_BLOOD);
}

/**
 * @brief Update _tail led positions
 * 
 */
void Snake::updateTail()
{
  for(unsigned int i=_wormLength-1; i>0; i--) {
    _tail[i].x = _tail[i-1].x;
    _tail[i].y = _tail[i-1].y;
  }
  _tail[0].x = _head.x;
  _tail[0].y = _head.y;

  for(unsigned int i=0; i<_wormLength; i++) {
    if (_tail[i].x > -1) {
      toggleLed(_tail[i].x, _tail[i].y, LED_TYPE_SNAKE);
    }
  }
}

/**
 * @brief Update _food position (generate new one if found)
 * 
 */
void Snake::updateFood()
{
  bool found = true;
  do {
    found = true;
    _food.x = random(0, X_MAX);
    _food.y = random(0, Y_MAX);
    for(unsigned int i=0; i<_wormLength; i++) {
      if (_tail[i].x == _food.x && _tail[i].y == _food.y) {
         found = false;
      }
    }
  } while(found == false);
  toggleLed(_food.x, _food.y, LED_TYPE_FOOD);
}

/**
 * @brief Check for collisison between snake and border or itself
 * 
 * @return true 
 * @return false 
 */
bool Snake::isCollision()
{
  if (_head.x < 0 || _head.x >= X_MAX) {
    return true;
  }
  if (_head.y < 0 || _head.y >= Y_MAX) {
    return true;
  }
  for(unsigned int i=1; i<_wormLength; i++) {
    if (_tail[i].x == _head.x && _tail[i].y == _head.y) {
       return true;
    }
  }
  return false;
}

/**
 * @brief Turn on LED on matrix
 * 
 * @param x x position of led
 * @param y y position of led
 * @param type type of pixel {SNAKE, OFF, FOOD, BLOOD}
 */
void Snake::toggleLed(uint8_t x, uint8_t y, uint8_t type)
{
  uint32_t color = LEDMatrix::Color24bit(0, 0, 0);

  switch(type) {
    case LED_TYPE_SNAKE:
      color = LEDMatrix::Color24bit(0, 100, 100);
      break;
    case LED_TYPE_EMPTY:
      color = LEDMatrix::Color24bit(0, 0, 0);
      break;
    case LED_TYPE_FOOD:
      color = LEDMatrix::Color24bit(0, 150, 0);
      break;
    case LED_TYPE_BLOOD:
      color = LEDMatrix::Color24bit(150, 0, 0);
      break;
  }

  (*_ledmatrix).gridAddPixel(x, y, color);
}