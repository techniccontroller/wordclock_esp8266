/**
 * @file pong.cpp
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class implementation for pong game
 * @version 0.1
 * @date 2022-03-06
 * 
 * @copyright Copyright (c) 2022
 * 
 * main code from https://elektro.turanis.de/html/prj041/index.html 
 * 
 */
#include "pong.h"

/**
 * @brief Construct a new Pong:: Pong object
 * 
 */
Pong::Pong(){

}

/**
 * @brief Construct a new Pong:: Pong object
 * 
 * @param myledmatrix pointer to LEDMatrix object, need to provide gridAddPixel(x, y, col), gridFlush()
 * @param mylogger pointer to UDPLogger object, need to provide a function logString(message)
 */
Pong::Pong(LEDMatrix *myledmatrix, UDPLogger *mylogger){
    _ledmatrix = myledmatrix;
    _logger = mylogger;
    _gameState = GAME_STATE_END;
}

/**
 * @brief Run main loop for one cycle
 * 
 */
void Pong::loopCycle(){
    switch(_gameState) {
        case GAME_STATE_INIT:
            initGame(2);
            break;
        case GAME_STATE_RUNNING:
            updateBall();
            updateGame();
            break;
        case GAME_STATE_END:
            break;
    }
}

/**
 * @brief Trigger control: UP for given player
 * 
 * @param playerid id of player {0, 1}
 */
void Pong::ctrlUp(uint8_t playerid){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_PONG) {
        _playerMovement[playerid] = PADDLE_MOVE_DOWN; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Trigger control: DOWN for given player
 * 
 * @param playerid id of player {0, 1}
 */
void Pong::ctrlDown(uint8_t playerid){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_PONG) {
        _playerMovement[playerid] = PADDLE_MOVE_UP; // need to swap direction as field is rotated 180deg
        _lastButtonClick = millis();
    }
}

/**
 * @brief Trigger control: NONE for given player
 * 
 * @param playerid id of player {0, 1}
 */
void Pong::ctrlNone(uint8_t playerid){
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_PONG) {
        _playerMovement[playerid] = PADDLE_MOVE_NONE;
        _lastButtonClick = millis();
    }
}

/**
 * @brief Initialize a new game
 * 
 * @param numBots number of bots {0, 1, 2} -> two bots results in animation
 */
void Pong::initGame(uint8_t numBots)
{
    (*_logger).logString("Pong: init with " + String(numBots) + " Bots");
    resetLEDs();
    _lastButtonClick = millis();

    _numBots = numBots;

    _ball.x = 1;
    _ball.y = (Y_MAX/2) - (PADDLE_WIDTH/2) + 1;
    _ball_old.x = _ball.x;
    _ball_old.y = _ball.y;
    _ballMovement[0] =  1;
    _ballMovement[1] = -1;
    _ballDelay = BALL_DELAY_MAX;

    for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
        _paddles[PLAYER_1][i].x = 0;
        _paddles[PLAYER_1][i].y = (Y_MAX/2) - (PADDLE_WIDTH/2) + i;
        _paddles[PLAYER_2][i].x = X_MAX - 1;
        _paddles[PLAYER_2][i].y = _paddles[PLAYER_1][i].y;
    }

    _gameState = GAME_STATE_RUNNING;
}

/**
 * @brief Update ball position
 * 
 */
void Pong::updateBall()
{
    bool hitBall = false;
    if ((millis() - _lastBallUpdate) < _ballDelay) {
        return;
    }
    _lastBallUpdate = millis();
    toggleLed(_ball.x, _ball.y, LED_TYPE_OFF);

    // collision detection for player 1
    if (_ballMovement[0] == -1 && _ball.x == 1) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
        if (_paddles[PLAYER_1][i].y == _ball.y) {
            hitBall = true;
            break;
        }
        }
    }

    // collision detection for player 2
    if (_ballMovement[0] == 1 && _ball.x == X_MAX-2) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
        if (_paddles[PLAYER_2][i].y == _ball.y) {
            hitBall = true;
            break;
        }
        }
    }

    if (hitBall == true) {
        _ballMovement[0] *= -1;
        if (_ballDelay > BALL_DELAY_MIN) {
        _ballDelay -= BALL_DELAY_STEP;
        }
    }

    _ball.x += _ballMovement[0];
    _ball.y += _ballMovement[1];

    if (_ball.x <=0 || _ball.x >= X_MAX-1) {
        endGame();
        return;
    }

    if (_ball.y <= 0 || _ball.y >= Y_MAX-1) {
        _ballMovement[1] *= -1;
    }

    toggleLed(_ball.x, _ball.y, LED_TYPE_BALL);
}

/**
 * @brief Game over, draw ball red
 * 
 */
void Pong::endGame()
{
    (*_logger).logString("Pong: Game ended");
    _gameState = GAME_STATE_END;
    toggleLed(_ball.x, _ball.y, LED_TYPE_BALL_RED);
}

/**
 * @brief Update paddle position and check for game over
 * 
 */
void Pong::updateGame()
{
    if ((millis() - _lastDrawUpdate) < GAME_DELAY_PONG) {
        return;
    }
    _lastDrawUpdate = millis();

    // turn off paddle LEDs
    for(uint8_t p=0; p<PLAYER_AMOUNT; p++) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
        toggleLed(_paddles[p][i].x, _paddles[p][i].y, LED_TYPE_OFF);
        }
    }

    // move _paddles
    for(uint8_t p=0; p<PLAYER_AMOUNT; p++) {
        uint8_t movement = getPlayerMovement(p);
        if (movement == PADDLE_MOVE_UP && _paddles[p][PADDLE_WIDTH-1].y < (Y_MAX-1)) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
            _paddles[p][i].y++;
        }
        }
        if (movement == PADDLE_MOVE_DOWN && _paddles[p][0].y > 0) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
            _paddles[p][i].y--;
        }
        }
    }

    // show paddle LEDs
    for(uint8_t p=0; p<PLAYER_AMOUNT; p++) {
        for(uint8_t i=0; i<PADDLE_WIDTH; i++) {
        toggleLed(_paddles[p][i].x, _paddles[p][i].y, LED_TYPE_PADDLE);
        }
    }
}

/**
 * @brief Get the next movement of paddle from given player
 * 
 * @param playerId id of player {0, 1}
 * @return uint8_t movement {UP, DOWN, NONE}
 */
uint8_t Pong::getPlayerMovement(uint8_t playerId)
{
    uint8_t action = PADDLE_MOVE_NONE;
    if(playerId < _numBots){
        // bot moves paddle
        int8_t ydir = _ball_old.y - _ball.y;
        int8_t diff = _paddles[playerId][PADDLE_WIDTH/2].y - _ball.y + ydir * 0.5;
        // no movement if ball moves away from paddle or no difference between ball and paddle
        if(diff == 0 || (_ballMovement[0] > 0 && playerId == 0) || (_ballMovement[0] < 0 && playerId == 1)){
            action = PADDLE_MOVE_NONE;
        }
        else if(diff > 0){
            action = PADDLE_MOVE_DOWN; 
        }
        else{
            action = PADDLE_MOVE_UP;
        }
    }
    else{
        action = _playerMovement[playerId];
        _playerMovement[playerId] = PADDLE_MOVE_NONE;
    }
    return action;
}

/**
 * @brief Clear the led matrix (turn all leds off)
 * 
 */
void Pong::resetLEDs()
{
    (*_ledmatrix).gridFlush();
}

/**
 * @brief Turn on LED on matrix
 * 
 * @param x x position of led
 * @param y y position of led
 * @param type type of pixel {PADDLE, BALL_RED, BALL, OFF}
 */
void Pong::toggleLed(uint8_t x, uint8_t y, uint8_t type)
{
    uint32_t color = LEDMatrix::Color24bit(0, 0, 0);

    switch(type) {
        case LED_TYPE_PADDLE:
            color = LEDMatrix::Color24bit(0, 80, 80);
            break;
        case LED_TYPE_BALL_RED:
            color = LEDMatrix::Color24bit(120, 0, 0);
            break;
        case LED_TYPE_BALL:
            color = LEDMatrix::Color24bit(0, 100, 0);
            break;
        case LED_TYPE_OFF:
            color = LEDMatrix::Color24bit(0, 0, 0);
            break;
    }

    (*_ledmatrix).gridAddPixel(x, y, color);
}