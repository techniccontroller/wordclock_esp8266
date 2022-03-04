#ifndef tetris_h
#define tetris_h

#include <Arduino.h>
#include "ledmatrix.h"
#include "udplogger.h"

#define DEBOUNCE_TIME 100
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


/////////////////////////////////////

class Tetris{

    public:
        Tetris();
        Tetris(LEDMatrix *myledmatrix, UDPLogger *mylogger);
        void onTetrisstartChange(boolean b);
        void onPlayChange(boolean b);
        void onPauseChange(boolean b);
        void onExitChange(boolean b);
        void onRechtsChange(boolean b);
        void onLinksChange(boolean b);
        void onHochChange(boolean b);
        void onRunterChange(boolean b);
        void onSpeedChange(int32_t i);
        void grundzustand();
        void resetLEDs();
        void tetrisInit();
        void printField();
        void projizieren(int x, int y, uint32_t color);

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
        void allesrot();
        void showscore();
        void loopCycle();

    private:

        LEDMatrix *ledmatrix;
        UDPLogger logger;
        Brick activeBrick;
        Field field;


        bool tnureinmal = true;
        long lastButtonClick = 0;
        int zustand = 0;
        int score = 0;
        int  frot = 0;
        int fgruen = 0;
        int fblau = 0;
        int gameStatet = GAME_STATE_INITt;
        uint16_t brickSpeed;
        unsigned long nbRowsThisLevel;
        unsigned long nbRowsTotal;

        long tonpause = 0;

        boolean tetrisGameOver;

        unsigned long prevUpdateTime = 0;
        unsigned long curTime;

        long tetrisshowscore;
        bool nureinmalscore = true;
        uint8_t thisselectedBrick = 0;
        uint8_t lastselectedBrick = 0;
        uint8_t selectedBrick;
        long zeitfallen = 0;
        int speedtetris = 80;
        long lastButtonClickr = 0;
        bool fallenerlaubt;

        Brick tmpBrick;

        //void (*addPixel)(uint8_t, uint8_t, uint32_t);
        void (*drawMatrix)();
        //void (*flushGrid)();

        
        // color library
        uint32_t colorLib[10] = {RED, GREEN, BLUE, YELLOW, CHOCOLATE, PURPLE, WHITE, AQUA, HOTPINK, DARKORANGE};

        // Brick "library"
        AbstractBrick brickLib[7] = {
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