/**
 * @file tetris.h
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class definition for tetris game
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright Copyright (c) 2022
 * 
 * main tetris code originally written by Klaas De Craemer, Ing. David Hrbaty
 * 
 */
#ifndef tetris_h
#define tetris_h

#include <Arduino.h>
#include "ledmatrix.h"
#include "udplogger.h"

#define DEBOUNCE_TIME_TETRIS 100
#define RED_END_TIME 1500
#define GAME_STATE_RUNNINGt 1
#define GAME_STATE_ENDt     2
#define GAME_STATE_INITt    3
#define GAME_STATE_PAUSEDt  4
#define GAME_STATE_READYt   5

//common
#define  DIR_UP    1
#define  DIR_DOWN  2
#define  DIR_LEFT  3
#define  DIR_RIGHT 4
//Maximum size of bricks. Individual bricks can still be smaller (eg 3x3)
#define  GREEN  0x008000
#define  RED    0xFF0000
#define  BLUE   0x0000FF
#define  YELLOW 0xFFFF00
#define  CHOCOLATE  0xD2691E
#define  PURPLE 0xFF00FF
#define  WHITE  0XFFFFFF
#define  AQUA   0x00FFFF
#define  HOTPINK 0xFF1493
#define  DARKORANGE 0xFF8C00

#define  MAX_BRICK_SIZE    4
#define  BRICKOFFSET       -1   // Y offset for new bricks

#define  INIT_SPEED        800  // Initial delay in ms between brick drops
#define  SPEED_STEP        10   // Factor for speed increase between levels, default 10
#define  LEVELUP           4    // Number of rows before levelup, default 5

#define WIDTH 11
#define HEIGHT 11

class Tetris{

    // Playing field
    struct Field {
        uint8_t pix[WIDTH][HEIGHT + 1]; //Make field one larger so that collision detection with bottom of field can be done in a uniform way
        uint32_t color[WIDTH][HEIGHT];
    };


    //Structure to represent active brick on screen
    struct Brick {
        boolean enabled;//Brick is disabled when it has landed
        int xpos, ypos;
        int yOffset;//Y-offset to use when placing brick at top of field
        uint8_t siz;
        uint8_t pix[MAX_BRICK_SIZE][MAX_BRICK_SIZE];

        uint32_t col;
    };

    //Struct to contain the different choices of blocks
    struct AbstractBrick {
        int yOffset;//Y-offset to use when placing brick at top of field
        uint8_t siz;
        uint8_t pix[MAX_BRICK_SIZE][MAX_BRICK_SIZE];
        uint32_t col;
    };

    public:
        Tetris();
        Tetris(LEDMatrix *myledmatrix, UDPLogger *mylogger);

        void ctrlStart();
        void ctrlPlayPause();
        void ctrlRight();
        void ctrlLeft();
        void ctrlUp();
        void ctrlDown();
        void setSpeed(uint8_t i);

        void loopCycle();

    private:
        void resetLEDs();
        void tetrisInit();
        void printField();

        /* *** Game functions *** */
        void newActiveBrick();
        boolean checkFieldCollision(struct Brick * brick);
        boolean checkSidesCollision(struct Brick * brick);
        void rotateActiveBrick();
        void shiftActiveBrick(int dir);
        void addActiveBrickToField();
        void moveFieldDownOne(uint8_t startRow);
        void checkFullLines();

        void clearField();
        void everythingRed();
        void showscore();


        LEDMatrix *_ledmatrix;
        UDPLogger *_logger;
        Brick _activeBrick;
        Field _field;

        unsigned long _lastButtonClick = 0;
        unsigned long _lastButtonClickr = 0;
        int _score = 0;
        int _gameStatet = GAME_STATE_INITt;
        unsigned int _brickSpeed;
        unsigned long _nbRowsThisLevel;
        unsigned long _nbRowsTotal;

        bool _tetrisGameOver;

        unsigned long _prevUpdateTime = 0;
        unsigned long _tetrisshowscoreTime = 0;
        unsigned long _dropTime = 0;
        unsigned int _speedtetris = 80;
        bool _allowdrop;
        
        // color library
        uint32_t _colorLib[10] = {RED, GREEN, BLUE, YELLOW, CHOCOLATE, PURPLE, WHITE, AQUA, HOTPINK, DARKORANGE};

        // Brick "library"
        AbstractBrick _brickLib[7] = {
            {
                1,//yoffset when adding brick to field
                4,
                { {0, 0, 0, 0},
                {0, 1, 1, 0},
                {0, 1, 1, 0},
                {0, 0, 0, 0}
                },
                WHITE
            },
            {
                0,
                4,
                { {0, 1, 0, 0},
                {0, 1, 0, 0},
                {0, 1, 0, 0},
                {0, 1, 0, 0}
                },
                GREEN
            },
            {
                1,
                3,
                { {0, 0, 0, 0},
                {1, 1, 1, 0},
                {0, 0, 1, 0},
                {0, 0, 0, 0}
                },
                BLUE
            },
            {
                1,
                3,
                { {0, 0, 1, 0},
                {1, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0}
                },
                YELLOW
            },
            {
                1,
                3,
                { {0, 0, 0, 0},
                {1, 1, 1, 0},
                {0, 1, 0, 0},
                {0, 0, 0, 0}
                },
                AQUA
            },
            {
                1,
                3,
                { {0, 1, 1, 0},
                {1, 1, 0, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0}
                },
                HOTPINK
            },
            {
                1,
                3,
                { {1, 1, 0, 0},
                {0, 1, 1, 0},
                {0, 0, 0, 0},
                {0, 0, 0, 0}
                },
                RED
            }
        };

};

#endif