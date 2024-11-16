/**
 * @file tetris.cpp
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class implementation for tetris game
 * @version 0.1
 * @date 2022-03-05
 * 
 * @copyright Copyright (c) 2022
 * 
 * main tetris code originally written by Klaas De Craemer, Ing. David Hrbaty
 * 
 */
#include "tetris.h"

Tetris::Tetris(){
}

/**
 * @brief Construct a new Tetris:: Tetris object
 * 
 * @param myledmatrix pointer to LEDMatrix object, need to provide gridAddPixel(x, y, col), drawOnMatrix(), gridFlush() and printNumber(x,y,n,col)
 * @param mylogger pointer to UDPLogger object, need to provide a function logString(message)
 */
Tetris::Tetris(LEDMatrix *myledmatrix, UDPLogger *mylogger){
    _logger = mylogger;
    _ledmatrix = myledmatrix;
    _gameStatet = GAME_STATE_READYt;
}

/**
 * @brief Run main loop for one cycle
 * 
 */
void Tetris::loopCycle(){
    switch (_gameStatet) {
        case GAME_STATE_READYt:

            break;
        case GAME_STATE_INITt:
            tetrisInit();

            break;
        case GAME_STATE_RUNNINGt:
            //If brick is still "on the loose", then move it down by one
            if (_activeBrick.enabled) {
                // move faster down when allow drop
                if (_allowdrop) {
                    if (millis() > _dropTime + 50) {
                        _dropTime = millis();
                        shiftActiveBrick(DIR_DOWN);
                        printField();
                    }
                }

                // move down with regular speed
                if ((millis() - _prevUpdateTime) > (_brickSpeed * _speedtetris / 100)) {
                        _prevUpdateTime = millis();
                        shiftActiveBrick(DIR_DOWN);
                        printField();
                }
            }
            else {
                _allowdrop = false;
                //Active brick has "crashed", check for full lines
                //and create new brick at top of field
                checkFullLines();
                newActiveBrick();
                _prevUpdateTime = millis();//Reset update time to avoid brick dropping two spaces
            }
            break;
        case GAME_STATE_PAUSEDt:

            break;
        case GAME_STATE_ENDt:
            // at game end show all bricks on field in red color for 1.5 seconds, then show score
            if (_tetrisGameOver == true) {
                _tetrisGameOver = false;
                (*_logger).logString("Tetris: end");
                everythingRed();
                _tetrisshowscoreTime = millis();
            }

            if (millis() > (_tetrisshowscoreTime + RED_END_TIME)) {
                resetLEDs();
                _score = _nbRowsTotal;
                showscore();
                _gameStatet = GAME_STATE_READYt;
            }
            break;
    }
}

/**
 * @brief Trigger control: START (& restart)
 * 
 */
void Tetris::ctrlStart() {
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_TETRIS)
    {
        _lastButtonClick = millis();
        _gameStatet = GAME_STATE_INITt;
    }
}

/**
 * @brief Trigger control: PAUSE/PLAY
 * 
 */
void Tetris::ctrlPlayPause() {
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_TETRIS)
    {
        _lastButtonClick = millis();
        if (_gameStatet == GAME_STATE_PAUSEDt) {
            (*_logger).logString("Tetris: continue");

            _gameStatet = GAME_STATE_RUNNINGt;

        } else if (_gameStatet == GAME_STATE_RUNNINGt) {
            (*_logger).logString("Tetris: pause");

            _gameStatet = GAME_STATE_PAUSEDt;
        }
    }
}

/**
 * @brief Trigger control: RIGHT
 * 
 */
void Tetris::ctrlRight() {
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_TETRIS && _gameStatet == GAME_STATE_RUNNINGt)
    {
        _lastButtonClick = millis();
        shiftActiveBrick(DIR_RIGHT);
        printField();
    }
}

/**
 * @brief Trigger control: LEFT
 * 
 */
void Tetris::ctrlLeft() {
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_TETRIS && _gameStatet == GAME_STATE_RUNNINGt)
    {
        _lastButtonClick = millis();
        shiftActiveBrick(DIR_LEFT);
        printField();
    }
}

/**
 * @brief Trigger control: UP (rotate)
 * 
 */
void Tetris::ctrlUp() {
    if (millis() > _lastButtonClick + DEBOUNCE_TIME_TETRIS && _gameStatet == GAME_STATE_RUNNINGt)
    {
        _lastButtonClick = millis();
        rotateActiveBrick();
        printField();
    }
}

/**
 * @brief Trigger control: DOWN (drop)
 * 
 */
void Tetris::ctrlDown() {
    // longer debounce time, to prevent immediate drop
    if (millis() > _lastButtonClickr + DEBOUNCE_TIME_TETRIS*5 && _gameStatet == GAME_STATE_RUNNINGt)
    {
        _allowdrop = true;
        _lastButtonClickr = millis();
    }
}

/**
 * @brief Set game speed
 * 
 * @param i new speed value (0 - 15)
 */
void Tetris::setSpeed(uint8_t i) {
    if(i > 15) i = 15;
    (*_logger).logString("setSpeed: " + String(i));
    _speedtetris = -10 * i + 150;
}

/**
 * @brief Clear the led matrix (turn all leds off)
 * 
 */
void Tetris::resetLEDs()
{
    (*_ledmatrix).gridFlush();
    (*_ledmatrix).drawOnMatrixInstant();
}

/**
 * @brief Initialize the tetris game
 * 
 */
void Tetris::tetrisInit() {
    (*_logger).logString("Tetris: init");
    
    clearField();
    _brickSpeed = INIT_SPEED;
    _nbRowsThisLevel = 0;
    _nbRowsTotal = 0;
    _tetrisGameOver = false;

    newActiveBrick();
    _prevUpdateTime = millis();

    _gameStatet = GAME_STATE_RUNNINGt;
}

/**
 * @brief Draw current field representation to led matrix
 * 
 */
void Tetris::printField() {
    int x, y;
    for (x = 0; x < WIDTH; x++) {
        for (y = 0; y < HEIGHT; y++) {
            uint8_t activeBrickPix = 0;
            if (_activeBrick.enabled) { //Only draw brick if it is enabled
                //Now check if brick is "in view"
                if ((x >= _activeBrick.xpos) && (x < (_activeBrick.xpos + (_activeBrick.siz)))
                        && (y >= _activeBrick.ypos) && (y < (_activeBrick.ypos + (_activeBrick.siz)))) {
                    activeBrickPix = (_activeBrick.pix)[x - _activeBrick.xpos][y - _activeBrick.ypos];
                }
            }
            if (_field.pix[x][y] == 1) {
                (*_ledmatrix).gridAddPixel(x, y, _field.color[x][y]);
            } else if (activeBrickPix == 1) {
                (*_ledmatrix).gridAddPixel(x, y, _activeBrick.col);
            } else {
                (*_ledmatrix).gridAddPixel(x, y, 0x000000);
            }
        }
    }
    (*_ledmatrix).drawOnMatrixInstant();
}


/* *** Game functions *** */
/**
 * @brief Spawn new (random) brick
 * 
 */
void Tetris::newActiveBrick() {
    uint8_t selectedBrick = 0;
    static uint8_t lastselectedBrick = 0;

    // choose random next brick, but not the same as before
    do {
        selectedBrick = random(7);
    }
    while (lastselectedBrick == selectedBrick);

    // Save selected brick for next round
    lastselectedBrick = selectedBrick;

    // every brick has its color, select corresponding color
    uint32_t selectedCol = _brickLib[selectedBrick].col;
    // Set properties of brick
    _activeBrick.siz = _brickLib[selectedBrick].siz;
    _activeBrick.yOffset = _brickLib[selectedBrick].yOffset;
    _activeBrick.xpos = WIDTH / 2 - _activeBrick.siz / 2;
    _activeBrick.ypos = BRICKOFFSET - _activeBrick.yOffset;
    _activeBrick.enabled = true;

    // Set color of brick
    _activeBrick.col = selectedCol;
    // _activeBrick.color = _colorLib[1];

    // Copy pix array of selected Brick
    uint8_t x, y;
    for (y = 0; y < MAX_BRICK_SIZE; y++) {
        for (x = 0; x < MAX_BRICK_SIZE; x++) {
            _activeBrick.pix[x][y] = (_brickLib[selectedBrick]).pix[x][y];
        }
    }

    // Check collision, if already, then game is over
    if (checkFieldCollision(&_activeBrick)) {
        _tetrisGameOver = true;
        _gameStatet = GAME_STATE_ENDt;

    }
}

/**
 * @brief Check collision between bricks in the field and the specified brick
 * 
 * @param brick brick to be checked for collision
 * @return boolean true if collision occured
 */
boolean Tetris::checkFieldCollision(struct Brick * brick) {
    uint8_t bx, by;
    uint8_t fx, fy;
    for (by = 0; by < MAX_BRICK_SIZE; by++) {
        for (bx = 0; bx < MAX_BRICK_SIZE; bx++) {
            fx = (*brick).xpos + bx;
            fy = (*brick).ypos + by;
            if (( (*brick).pix[bx][by] == 1)
                    && ( _field.pix[fx][fy] == 1)) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Check collision between specified brick and all sides of the playing field
 * 
 * @param brick brick to be checked for collision
 * @return boolean true if collision occured
 */
boolean Tetris::checkSidesCollision(struct Brick * brick) {
    //Check vertical collision with sides of field
    uint8_t bx, by;
    int8_t fx;
    for (by = 0; by < MAX_BRICK_SIZE; by++) {
        for (bx = 0; bx < MAX_BRICK_SIZE; bx++) {
            if ( (*brick).pix[bx][by] == 1) {
                fx = (*brick).xpos + bx;//Determine actual position in the field of the current pix of the brick
                if (fx < 0 || fx >= WIDTH) {
                    return true;
                }
            }
        }
    }
    return false;
}

/**
 * @brief Rotate current active brick
 * 
 */
void Tetris::rotateActiveBrick() {
    //Copy active brick pix array to temporary pix array
    uint8_t x, y;
    Brick tmpBrick;
    for (y = 0; y < MAX_BRICK_SIZE; y++) {
        for (x = 0; x < MAX_BRICK_SIZE; x++) {
            tmpBrick.pix[x][y] = _activeBrick.pix[x][y];
        }
    }
    tmpBrick.xpos = _activeBrick.xpos;
    tmpBrick.ypos = _activeBrick.ypos;
    tmpBrick.siz = _activeBrick.siz;

    //Depending on size of the active brick, we will rotate differently
    if (_activeBrick.siz == 3) {
        //Perform rotation around center pix
        tmpBrick.pix[0][0] = _activeBrick.pix[0][2];
        tmpBrick.pix[0][1] = _activeBrick.pix[1][2];
        tmpBrick.pix[0][2] = _activeBrick.pix[2][2];
        tmpBrick.pix[1][0] = _activeBrick.pix[0][1];
        tmpBrick.pix[1][1] = _activeBrick.pix[1][1];
        tmpBrick.pix[1][2] = _activeBrick.pix[2][1];
        tmpBrick.pix[2][0] = _activeBrick.pix[0][0];
        tmpBrick.pix[2][1] = _activeBrick.pix[1][0];
        tmpBrick.pix[2][2] = _activeBrick.pix[2][0];
        //Keep other parts of temporary block clear
        tmpBrick.pix[0][3] = 0;
        tmpBrick.pix[1][3] = 0;
        tmpBrick.pix[2][3] = 0;
        tmpBrick.pix[3][3] = 0;
        tmpBrick.pix[3][2] = 0;
        tmpBrick.pix[3][1] = 0;
        tmpBrick.pix[3][0] = 0;

    } else if (_activeBrick.siz == 4) {
        //Perform rotation around center "cross"
        tmpBrick.pix[0][0] = _activeBrick.pix[0][3];
        tmpBrick.pix[0][1] = _activeBrick.pix[1][3];
        tmpBrick.pix[0][2] = _activeBrick.pix[2][3];
        tmpBrick.pix[0][3] = _activeBrick.pix[3][3];
        tmpBrick.pix[1][0] = _activeBrick.pix[0][2];
        tmpBrick.pix[1][1] = _activeBrick.pix[1][2];
        tmpBrick.pix[1][2] = _activeBrick.pix[2][2];
        tmpBrick.pix[1][3] = _activeBrick.pix[3][2];
        tmpBrick.pix[2][0] = _activeBrick.pix[0][1];
        tmpBrick.pix[2][1] = _activeBrick.pix[1][1];
        tmpBrick.pix[2][2] = _activeBrick.pix[2][1];
        tmpBrick.pix[2][3] = _activeBrick.pix[3][1];
        tmpBrick.pix[3][0] = _activeBrick.pix[0][0];
        tmpBrick.pix[3][1] = _activeBrick.pix[1][0];
        tmpBrick.pix[3][2] = _activeBrick.pix[2][0];
        tmpBrick.pix[3][3] = _activeBrick.pix[3][0];
    } else {
        (*_logger).logString("Tetris: Brick size error");
    }

    // Now validate by checking collision.
    // Collision possibilities:
    //   - Brick now sticks outside field
    //   - Brick now sticks inside fixed bricks of field
    // In case of collision, we just discard the rotated temporary brick
    if ((!checkSidesCollision(&tmpBrick)) && (!checkFieldCollision(&tmpBrick))) {
        //Copy temporary brick pix array to active pix array
        for (y = 0; y < MAX_BRICK_SIZE; y++) {
            for (x = 0; x < MAX_BRICK_SIZE; x++) {
                _activeBrick.pix[x][y] = tmpBrick.pix[x][y];
            }
        }
    }
}

/**
 * @brief Shift brick left/right/down by one if possible
 * 
 * @param dir direction to be shifted
 */
void Tetris::shiftActiveBrick(int dir) {
    // Change position of active brick (no copy to temporary needed)
    if (dir == DIR_LEFT) {
        _activeBrick.xpos--;
    } else if (dir == DIR_RIGHT) {
        _activeBrick.xpos++;
    } else if (dir == DIR_DOWN) {
        _activeBrick.ypos++;
    }

    // Check position of active brick
    // Two possibilities when collision is detected:
    //   - Direction was LEFT/RIGHT, just revert position back
    //   - Direction was DOWN, revert position and fix block to field on collision
    // When no collision, keep _activeBrick coordinates
    if ((checkSidesCollision(&_activeBrick)) || (checkFieldCollision(&_activeBrick))) {
        if (dir == DIR_LEFT) {
            _activeBrick.xpos++;
        } else if (dir == DIR_RIGHT) {
            _activeBrick.xpos--;
        } else if (dir == DIR_DOWN) {
            _activeBrick.ypos--;// Go back up one
            addActiveBrickToField();
            _activeBrick.enabled = false;// Disable brick, it is no longer moving
        }
    }
}

/**
 * @brief Copy active pixels to field, including color
 * 
 */
void Tetris::addActiveBrickToField() {
    uint8_t bx, by;
    int8_t fx, fy;
    for (by = 0; by < MAX_BRICK_SIZE; by++) {
        for (bx = 0; bx < MAX_BRICK_SIZE; bx++) {
            fx = _activeBrick.xpos + bx;
            fy = _activeBrick.ypos + by;

            if (fx >= 0 && fy >= 0 && fx < WIDTH && fy < HEIGHT && _activeBrick.pix[bx][by]) { // Check if inside playing field
                // _field.pix[fx][fy] = _field.pix[fx][fy] || _activeBrick.pix[bx][by];
                _field.pix[fx][fy] = _activeBrick.pix[bx][by];
                _field.color[fx][fy] = _activeBrick.col;
            }
        }
    }
}

/**
 * @brief Move all pix from the field above startRow down by one. startRow is overwritten
 * 
 * @param startRow 
 */
void Tetris::moveFieldDownOne(uint8_t startRow) {
    if (startRow == 0) { // Topmost row has nothing on top to move...
        return;
    }
    uint8_t x, y;
    for (y = startRow - 1; y > 0; y--) {
        for (x = 0; x < WIDTH; x++) {
            _field.pix[x][y + 1] = _field.pix[x][y];
            _field.color[x][y + 1] = _field.color[x][y];
        }
    }
}

/**
 * @brief Check if a line is complete
 * 
 */
void Tetris::checkFullLines() {
    int x, y;
    int minY = 0;
    for (y = (HEIGHT - 1); y >= minY; y--) {
        uint8_t rowSum = 0;
        for (x = 0; x < WIDTH; x++) {
            rowSum = rowSum + (_field.pix[x][y]);
        }
        if (rowSum >= WIDTH) {
            // Found full row, animate its removal
            _activeBrick.enabled = false;

            for (x = 0; x < WIDTH; x++) {
                _field.pix[x][y] = 0;
                printField();
                delay(100);
            }
            // Move all upper rows down by one
            moveFieldDownOne(y);
            y++; minY++;
            printField();
            delay(100);


            _nbRowsThisLevel++; _nbRowsTotal++;
            if (_nbRowsThisLevel >= LEVELUP) {
                _nbRowsThisLevel = 0;
                _brickSpeed = _brickSpeed - SPEED_STEP;
                if (_brickSpeed < 200) {
                    _brickSpeed = 200;
                }
            }
        }
    }
}

/**
 * @brief Clear field
 * 
 */
void Tetris::clearField() {
    uint8_t x, y;
    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            _field.pix[x][y] = 0;
            _field.color[x][y] = 0;
        }
    }
    for (x = 0; x < WIDTH; x++) { //This last row is invisible to the player and only used for the collision detection routine
        _field.pix[x][HEIGHT] = 1;
    }
}

/**
 * @brief Color all bricks on the field red
 * 
 */
void Tetris::everythingRed() {
    int x, y;
    for (x = 0; x < WIDTH; x++) {
        for (y = 0; y < HEIGHT; y++) {
            uint8_t activeBrickPix = 0;
            if (_activeBrick.enabled) { //Only draw brick if it is enabled
                //Now check if brick is "in view"
                if ((x >= _activeBrick.xpos) && (x < (_activeBrick.xpos + (_activeBrick.siz)))
                        && (y >= _activeBrick.ypos) && (y < (_activeBrick.ypos + (_activeBrick.siz)))) {
                    activeBrickPix = (_activeBrick.pix)[x - _activeBrick.xpos][y - _activeBrick.ypos];
                }
            }
            if (_field.pix[x][y] == 1) {
                (*_ledmatrix).gridAddPixel(x, y, RED);
            } else if (activeBrickPix == 1) {
                (*_ledmatrix).gridAddPixel(x, y, RED);
            } else {
                (*_ledmatrix).gridAddPixel(x, y, 0x000000);
            }
        }
    }
    (*_ledmatrix).drawOnMatrixInstant();
}

/**
 * @brief Draw score to led matrix
 * 
 */
void Tetris::showscore() {
    uint32_t color = LEDMatrix::Color24bit(255, 170, 0);
    (*_ledmatrix).gridFlush();
    if(_score > 9){
        (*_ledmatrix).printNumber(2, 3, _score/10, color);
        (*_ledmatrix).printNumber(6, 3, _score%10, color);
    }else{
        (*_ledmatrix).printNumber(4, 3, _score, color);
    }
    (*_ledmatrix).drawOnMatrixInstant();
}