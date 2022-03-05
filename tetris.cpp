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
 * @param myledmatrix pointer to ledmatrix object, need to provide gridAddPixel(x, y, col), drawOnMatrix(), gridFlush() and printNumber(x,y,n,col)
 * @param mylogger pointer to UDPLogger object, need to provide a function logString(message)
 */
Tetris::Tetris(LEDMatrix *myledmatrix, UDPLogger *mylogger){
    logger = *mylogger;
    ledmatrix = myledmatrix;
    gameStatet = GAME_STATE_READYt;
}

/**
 * @brief Run main loop for one cycle
 * 
 */
void Tetris::loopCycle(){
    switch (gameStatet) {
        case GAME_STATE_READYt:

            break;
        case GAME_STATE_INITt:
            tetrisInit();

            break;
        case GAME_STATE_RUNNINGt:
            //If brick is still "on the loose", then move it down by one
            if (activeBrick.enabled) {
                // move faster down when allow drop
                if (allowdrop) {
                    if (millis() > droptime + 50) {
                        droptime = millis();
                        shiftActiveBrick(DIR_DOWN);
                        printField();
                    }
                }

                // move down with regular speed
                if ((millis() - prevUpdateTime) > (brickSpeed * speedtetris / 100)) {
                        prevUpdateTime = millis();
                        shiftActiveBrick(DIR_DOWN);
                        printField();
                }
            }
            else {
                allowdrop = false;
                //Active brick has "crashed", check for full lines
                //and create new brick at top of field
                checkFullLines();
                newActiveBrick();
                prevUpdateTime = millis();//Reset update time to avoid brick dropping two spaces
            }
            break;
        case GAME_STATE_PAUSEDt:

            break;
        case GAME_STATE_ENDt:
            // at game end show all bricks on field in red color for 1.5 seconds, then show score
            if (tetrisGameOver == true) {
                tetrisGameOver = false;
                logger.logString("Tetris: end");
                everythingRed();
                tetrisshowscore = millis();
            }

            if (millis() > (tetrisshowscore + RED_END_TIME)) {
                resetLEDs();
                score = nbRowsTotal;
                showscore();
                gameStatet = GAME_STATE_READYt;
            }
            break;
    }
}

/**
 * @brief Trigger control: START (& restart)
 * 
 */
void Tetris::ctrlStart() {
    if (millis() > lastButtonClick + DEBOUNCE_TIME)
    {
        lastButtonClick = millis();
        gameStatet = GAME_STATE_INITt;
    }
}

/**
 * @brief Trigger control: PAUSE/PLAY
 * 
 */
void Tetris::ctrlPlayPause() {
    if (millis() > lastButtonClick + DEBOUNCE_TIME)
    {
        lastButtonClick = millis();
        if (gameStatet == GAME_STATE_PAUSEDt) {
            logger.logString("Tetris: continue");

            gameStatet = GAME_STATE_RUNNINGt;

        } else if (gameStatet == GAME_STATE_RUNNINGt) {
            logger.logString("Tetris: pause");

            gameStatet = GAME_STATE_PAUSEDt;
        }
    }
}

/**
 * @brief Trigger control: RIGHT
 * 
 */
void Tetris::ctrlRight() {
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameStatet == GAME_STATE_RUNNINGt)
    {
        lastButtonClick = millis();
        shiftActiveBrick(DIR_RIGHT);
        printField();
    }
}

/**
 * @brief Trigger control: LEFT
 * 
 */
void Tetris::ctrlLeft() {
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameStatet == GAME_STATE_RUNNINGt)
    {
        lastButtonClick = millis();
        shiftActiveBrick(DIR_LEFT);
        printField();
    }
}

/**
 * @brief Trigger control: UP (rotate)
 * 
 */
void Tetris::ctrlUp() {
    if (millis() > lastButtonClick + DEBOUNCE_TIME && gameStatet == GAME_STATE_RUNNINGt)
    {
        lastButtonClick = millis();
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
    if (millis() > lastButtonClickr + DEBOUNCE_TIME*5 && gameStatet == GAME_STATE_RUNNINGt)
    {
        allowdrop = true;
        lastButtonClickr = millis();
    }
}

/**
 * @brief Set game speed
 * 
 * @param i new speed value
 */
void Tetris::setSpeed(int32_t i) {
    logger.logString("setSpeed: " + String(i));
    speedtetris = -10 * i + 150;
}

/**
 * @brief Clear the led matrix (turn all leds off)
 * 
 */
void Tetris::resetLEDs()
{
    (*ledmatrix).gridFlush();
    (*ledmatrix).drawOnMatrixInstant();
}

/**
 * @brief Initialize the tetris game
 * 
 */
void Tetris::tetrisInit() {
    logger.logString("Tetris: init");
    
    clearField();
    brickSpeed = INIT_SPEED;
    nbRowsThisLevel = 0;
    nbRowsTotal = 0;
    tetrisGameOver = false;

    newActiveBrick();
    prevUpdateTime = millis();

    gameStatet = GAME_STATE_RUNNINGt;
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
            if (activeBrick.enabled) { //Only draw brick if it is enabled
                //Now check if brick is "in view"
                if ((x >= activeBrick.xpos) && (x < (activeBrick.xpos + (activeBrick.siz)))
                        && (y >= activeBrick.ypos) && (y < (activeBrick.ypos + (activeBrick.siz)))) {
                    activeBrickPix = (activeBrick.pix)[x - activeBrick.xpos][y - activeBrick.ypos];
                }
            }
            if (field.pix[x][y] == 1) {
                (*ledmatrix).gridAddPixel(x, y, field.color[x][y]);
            } else if (activeBrickPix == 1) {
                (*ledmatrix).gridAddPixel(x, y, activeBrick.col);
            } else {
                (*ledmatrix).gridAddPixel(x, y, 0x000000);
            }
        }
    }
    (*ledmatrix).drawOnMatrixInstant();
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
    uint32_t selectedCol = brickLib[selectedBrick].col;
    // Set properties of brick
    activeBrick.siz = brickLib[selectedBrick].siz;
    activeBrick.yOffset = brickLib[selectedBrick].yOffset;
    activeBrick.xpos = WIDTH / 2 - activeBrick.siz / 2;
    activeBrick.ypos = BRICKOFFSET - activeBrick.yOffset;
    activeBrick.enabled = true;

    // Set color of brick
    activeBrick.col = selectedCol;
    // activeBrick.color = colorLib[1];

    // Copy pix array of selected Brick
    uint8_t x, y;
    for (y = 0; y < MAX_BRICK_SIZE; y++) {
        for (x = 0; x < MAX_BRICK_SIZE; x++) {
            activeBrick.pix[x][y] = (brickLib[selectedBrick]).pix[x][y];
        }
    }

    // Check collision, if already, then game is over
    if (checkFieldCollision(&activeBrick)) {
        tetrisGameOver = true;
        gameStatet = GAME_STATE_ENDt;

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
                    && ( field.pix[fx][fy] == 1)) {
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
    uint8_t fx, fy;
    for (by = 0; by < MAX_BRICK_SIZE; by++) {
        for (bx = 0; bx < MAX_BRICK_SIZE; bx++) {
            if ( (*brick).pix[bx][by] == 1) {
                fx = (*brick).xpos + bx;//Determine actual position in the field of the current pix of the brick
                fy = (*brick).ypos + by;
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
            tmpBrick.pix[x][y] = activeBrick.pix[x][y];
        }
    }
    tmpBrick.xpos = activeBrick.xpos;
    tmpBrick.ypos = activeBrick.ypos;
    tmpBrick.siz = activeBrick.siz;

    //Depending on size of the active brick, we will rotate differently
    if (activeBrick.siz == 3) {
        //Perform rotation around center pix
        tmpBrick.pix[0][0] = activeBrick.pix[0][2];
        tmpBrick.pix[0][1] = activeBrick.pix[1][2];
        tmpBrick.pix[0][2] = activeBrick.pix[2][2];
        tmpBrick.pix[1][0] = activeBrick.pix[0][1];
        tmpBrick.pix[1][1] = activeBrick.pix[1][1];
        tmpBrick.pix[1][2] = activeBrick.pix[2][1];
        tmpBrick.pix[2][0] = activeBrick.pix[0][0];
        tmpBrick.pix[2][1] = activeBrick.pix[1][0];
        tmpBrick.pix[2][2] = activeBrick.pix[2][0];
        //Keep other parts of temporary block clear
        tmpBrick.pix[0][3] = 0;
        tmpBrick.pix[1][3] = 0;
        tmpBrick.pix[2][3] = 0;
        tmpBrick.pix[3][3] = 0;
        tmpBrick.pix[3][2] = 0;
        tmpBrick.pix[3][1] = 0;
        tmpBrick.pix[3][0] = 0;

    } else if (activeBrick.siz == 4) {
        //Perform rotation around center "cross"
        tmpBrick.pix[0][0] = activeBrick.pix[0][3];
        tmpBrick.pix[0][1] = activeBrick.pix[1][3];
        tmpBrick.pix[0][2] = activeBrick.pix[2][3];
        tmpBrick.pix[0][3] = activeBrick.pix[3][3];
        tmpBrick.pix[1][0] = activeBrick.pix[0][2];
        tmpBrick.pix[1][1] = activeBrick.pix[1][2];
        tmpBrick.pix[1][2] = activeBrick.pix[2][2];
        tmpBrick.pix[1][3] = activeBrick.pix[3][2];
        tmpBrick.pix[2][0] = activeBrick.pix[0][1];
        tmpBrick.pix[2][1] = activeBrick.pix[1][1];
        tmpBrick.pix[2][2] = activeBrick.pix[2][1];
        tmpBrick.pix[2][3] = activeBrick.pix[3][1];
        tmpBrick.pix[3][0] = activeBrick.pix[0][0];
        tmpBrick.pix[3][1] = activeBrick.pix[1][0];
        tmpBrick.pix[3][2] = activeBrick.pix[2][0];
        tmpBrick.pix[3][3] = activeBrick.pix[3][0];
    } else {
        logger.logString("Tetris: Brick size error");
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
                activeBrick.pix[x][y] = tmpBrick.pix[x][y];
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
        activeBrick.xpos--;
    } else if (dir == DIR_RIGHT) {
        activeBrick.xpos++;
    } else if (dir == DIR_DOWN) {
        activeBrick.ypos++;
    }

    // Check position of active brick
    // Two possibilities when collision is detected:
    //   - Direction was LEFT/RIGHT, just revert position back
    //   - Direction was DOWN, revert position and fix block to field on collision
    // When no collision, keep activeBrick coordinates
    if ((checkSidesCollision(&activeBrick)) || (checkFieldCollision(&activeBrick))) {
        if (dir == DIR_LEFT) {
            activeBrick.xpos++;
        } else if (dir == DIR_RIGHT) {
            activeBrick.xpos--;
        } else if (dir == DIR_DOWN) {
            activeBrick.ypos--;// Go back up one
            addActiveBrickToField();
            activeBrick.enabled = false;// Disable brick, it is no longer moving
        }
    }
}

/**
 * @brief Copy active pixels to field, including color
 * 
 */
void Tetris::addActiveBrickToField() {
    uint8_t bx, by;
    uint8_t fx, fy;
    for (by = 0; by < MAX_BRICK_SIZE; by++) {
        for (bx = 0; bx < MAX_BRICK_SIZE; bx++) {
            fx = activeBrick.xpos + bx;
            fy = activeBrick.ypos + by;

            if (fx >= 0 && fy >= 0 && fx < WIDTH && fy < HEIGHT && activeBrick.pix[bx][by]) { // Check if inside playing field
                // field.pix[fx][fy] = field.pix[fx][fy] || activeBrick.pix[bx][by];
                field.pix[fx][fy] = activeBrick.pix[bx][by];
                field.color[fx][fy] = activeBrick.col;
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
            field.pix[x][y + 1] = field.pix[x][y];
            field.color[x][y + 1] = field.color[x][y];
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
            rowSum = rowSum + (field.pix[x][y]);
        }
        if (rowSum >= WIDTH) {
            // Found full row, animate its removal
            activeBrick.enabled = false;

            for (x = 0; x < WIDTH; x++) {
                field.pix[x][y] = 0;
                printField();
                delay(100);
            }
            // Move all upper rows down by one
            moveFieldDownOne(y);
            y++; minY++;
            printField();
            delay(100);


            nbRowsThisLevel++; nbRowsTotal++;
            if (nbRowsThisLevel >= LEVELUP) {
                nbRowsThisLevel = 0;
                brickSpeed = brickSpeed - SPEED_STEP;
                if (brickSpeed < 200) {
                    brickSpeed = 200;
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
            field.pix[x][y] = 0;
            field.color[x][y] = 0;
        }
    }
    for (x = 0; x < WIDTH; x++) { //This last row is invisible to the player and only used for the collision detection routine
        field.pix[x][HEIGHT] = 1;
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
            if (activeBrick.enabled) { //Only draw brick if it is enabled
                //Now check if brick is "in view"
                if ((x >= activeBrick.xpos) && (x < (activeBrick.xpos + (activeBrick.siz)))
                        && (y >= activeBrick.ypos) && (y < (activeBrick.ypos + (activeBrick.siz)))) {
                    activeBrickPix = (activeBrick.pix)[x - activeBrick.xpos][y - activeBrick.ypos];
                }
            }
            if (field.pix[x][y] == 1) {
                (*ledmatrix).gridAddPixel(x, y, RED);
            } else if (activeBrickPix == 1) {
                (*ledmatrix).gridAddPixel(x, y, RED);
            } else {
                (*ledmatrix).gridAddPixel(x, y, 0x000000);
            }
        }
    }
    (*ledmatrix).drawOnMatrixInstant();
}

/**
 * @brief Draw score to led matrix
 * 
 */
void Tetris::showscore() {
    uint32_t color = LEDMatrix::Color24bit(255, 170, 0);
    (*ledmatrix).gridFlush();
    if(score > 9){
        (*ledmatrix).printNumber(2, 3, score/10, color);
        (*ledmatrix).printNumber(6, 3, score%10, color);
    }else{
        (*ledmatrix).printNumber(4, 3, score, color);
    }
    (*ledmatrix).drawOnMatrixInstant();
}