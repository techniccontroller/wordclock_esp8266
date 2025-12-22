#define MILLIS_PER_MINUTE 60000
#define NUM_LED_PER_SECOND 1  // Nessi 2 LED pro Sekunde

#include "ledring.h"

/**
 * @brief Turn on the frame light with given color
 *
 * @param color
 */
void turnOnFrameLight(uint32_t color)
{
    frameLED.fill(color);
    frameLED.drawOnRingInstant();
}

/**
 * @brief Turn off the frame light
 *
 */
void turnOffFrameLight()
{
    frameLED.flushRing();
    frameLED.drawOnRingInstant();
}

/**
 * * @brief Show the seconds on the frame light
 * 
 * @param minutes current minutes
 */
void showSeconds(uint8_t minutes) // use minutes to calculate the seconds (more control on the behavior, can also display subseconds)
{
    static uint8_t lastMinutes = 0;
    static uint32_t timeOfLastMinuteChange = 0;
    static uint32_t black = LEDRing::Color24bit(0, 0, 0);
    static uint8_t incDecCycle = 0;

    // check if minutes changed
    if(minutes != lastMinutes) {
        timeOfLastMinuteChange = millis();
        lastMinutes = minutes;
        incDecCycle = (incDecCycle + 1) % 2;
    }

    // calculate time since last minute change
    uint32_t timeSinceLastMinuteChange = millis() - timeOfLastMinuteChange;

    // calculate seconds progress
    float progressSeconds = (float)timeSinceLastMinuteChange / (float)MILLIS_PER_MINUTE;
    int activePixelSeconds = (int)(progressSeconds * RING_LED_COUNT);

    frameLED.flushRing();

    if(frameSecondsSingle){ // single mode
        for(int i = 0; i < RING_LED_COUNT; i++)
        {
            if(i == activePixelSeconds){
                for(int j = 0; j < NUM_LED_PER_SECOND; j++)
                {
                    frameLED.setPixel((i - j + RING_LED_COUNT) % RING_LED_COUNT, maincolor_clock);
                }
            }
        }
    }
    else {
        for(int i = 0; i < RING_LED_COUNT; i++)
        {
            if(!frameSecondsIncDecCycle){ // increment only mode
                if(i <= activePixelSeconds){
                    frameLED.setPixel(i, maincolor_clock);
                }
                else{
                    frameLED.setPixel(i, black);
                }
            }
            else { // increment + decrement mode
                if(incDecCycle == 0){ // increment
                    if(i <= activePixelSeconds){
                        frameLED.setPixel(i, maincolor_clock);
                    }
                    else{
                        frameLED.setPixel(i, black);
                    }
                }
                else { // decrement
                    if(i >= activePixelSeconds){
                        frameLED.setPixel(i, maincolor_clock);
                    }
                    else{
                        frameLED.setPixel(i, black);
                    }
                }
            }
        }
    }
    frameLED.drawOnRingInstant();
}

/**
 * * @brief Update the frame light
 */
void updateFrame()
{
    if(nightMode || !frameLightActive)
    {
        turnOffFrameLight();
        return;
    }

    if(frameLightActive && !frameSecondsActive)
    {
        turnOnFrameLight(maincolor_clock);
        return;
    }
    
    uint8_t minutes = ntp.getMinutes();
    showSeconds(minutes);
}