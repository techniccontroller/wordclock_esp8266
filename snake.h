/**
 * @file snake.h
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class declaration of snake game
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright Copyright (c) 2022
 * 
 * main code from https://elektro.turanis.de/html/prj099/index.html
 * 
 */
#ifndef snake_h
#define snake_h

#include <Arduino.h>
#include "ledmatrix.h"
#include "udplogger.h"

#define DEBOUNCE_TIME 300   // in ms

#define X_MAX 11
#define Y_MAX 11

#define GAME_DELAY 400      // in ms

#define LED_TYPE_SNAKE 1
#define LED_TYPE_OFF   2
#define LED_TYPE_FOOD  3
#define LED_TYPE_BLOOD 4

#define DIRECTION_NONE  0
#define DIRECTION_UP    1
#define DIRECTION_DOWN  2
#define DIRECTION_LEFT  3
#define DIRECTION_RIGHT 4

#define GAME_STATE_RUNNING 1
#define GAME_STATE_END     2
#define GAME_STATE_INIT    3

#define MAX_TAIL_LENGTH X_MAX * Y_MAX
#define MIN_TAIL_LENGTH 3

// Define frequencies for some musical notes
#define NOTE_C4  261
#define NOTE_D4  294
#define NOTE_E4  329
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

class Snake{

    struct Coords {
      int x;
      int y;
    };

    public:
        Snake();
        Snake(LEDMatrix *myledmatrix, UDPLogger *mylogger, uint8_t soundPin);
        void loopCycle();
        void initGame();
        void ctrlUp();
        void ctrlDown();
        void ctrlLeft();
        void ctrlRight();
        
    private:
        LEDMatrix *_ledmatrix;
        UDPLogger *_logger;
        uint8_t _userDirection;
        uint8_t _gameState;
        Coords _head;
        Coords _tail[MAX_TAIL_LENGTH];
        Coords _food;
        unsigned long _lastDrawUpdate = 0;
        unsigned long _lastButtonClick;
        unsigned int _wormLength = 0;
        uint8_t _soundPin;

        void resetLEDs();
        void updateGame();
        void endGame();
        void updateTail();
        void updateFood();
        bool isCollision();
        void toggleLed(uint8_t x, uint8_t y, uint8_t type);
        void playTone(uint16_t frequency, uint32_t duration);
        void playEatMelody();
        void playGameOverMelody();

};

#endif