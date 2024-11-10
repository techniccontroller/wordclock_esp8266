/**
 * @file pong.h
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class declaration for pong game
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright Copyright (c) 2022
 * 
 * main code from https://elektro.turanis.de/html/prj041/index.html 
 * 
 */

#ifndef pong_h
#define pong_h

#include <Arduino.h>
#include "ledmatrix.h"
#include "udplogger.h"

#define DEBOUNCE_TIME 10  // in ms

#define X_MAX 11
#define Y_MAX 11

#define GAME_DELAY 80         // in ms
#define BALL_DELAY_MAX   350  // in ms
#define BALL_DELAY_MIN    50  // in ms
#define BALL_DELAY_STEP    5  // in ms

#define PLAYER_AMOUNT 2
#define PLAYER_1 0
#define PLAYER_2 1

#define PADDLE_WIDTH 3

#define PADDLE_MOVE_NONE  0
#define PADDLE_MOVE_UP    1
#define PADDLE_MOVE_DOWN  2

#define LED_TYPE_OFF      1
#define LED_TYPE_PADDLE   2
#define LED_TYPE_BALL     3
#define LED_TYPE_BALL_RED 4

#define GAME_STATE_RUNNING 1
#define GAME_STATE_END     2
#define GAME_STATE_INIT    3

// Define frequencies for some musical notes
#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

class Pong{

    struct Coords {
        uint8_t x;
        uint8_t y;
    };

    public:
        Pong();
        Pong(LEDMatrix *myledmatrix, UDPLogger *mylogger, uint8_t soundPin);
        void loopCycle();
        void initGame(uint8_t numBots);
        void ctrlUp(uint8_t playerid);
        void ctrlDown(uint8_t playerid);
        void ctrlNone(uint8_t playerid);
    
    private:
        LEDMatrix *_ledmatrix;
        UDPLogger *_logger;
        uint8_t _gameState;
        uint8_t _numBots;
        uint8_t _playerMovement[PLAYER_AMOUNT];
        Coords _paddles[PLAYER_AMOUNT][PADDLE_WIDTH];
        Coords _ball;
        Coords _ball_old;
        int _ballMovement[2];
        unsigned int _ballDelay;
        unsigned long _lastDrawUpdate = 0;
        unsigned long _lastBallUpdate = 0;
        unsigned long _lastButtonClick = 0;
        uint8_t _soundPin;
        

        void updateBall();
        void endGame();
        void updateGame();
        uint8_t getPlayerMovement(uint8_t playerId);
        void resetLEDs();
        void toggleLed(uint8_t x, uint8_t y, uint8_t type);
        void playTone(uint16_t frequency, uint32_t duration);
        void playBatHitMelody();
        void playWallHitMelody();
        void playGameOverMelody();
};

#endif