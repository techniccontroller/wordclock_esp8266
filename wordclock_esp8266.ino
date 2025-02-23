/**
 * Wordclock 2.0 - Wordclock with ESP8266 and NTP time update
 * 
 * created by techniccontroller 04.12.2021
 * 
 * components:
 * - ESP8266
 * - Neopixelstrip
 * 
 * Board settings:
 * - Board: NodeMCU 1.0 (ESP-12E Module)
 * - Flash Size: 4MB (FS:2MB OTA:~1019KB)
 * - Upload Speed: 115200
 *  
 * 
 * with code parts from:
 * - Adafruit NeoPixel strandtest.ino, https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
 * - Esp8266 und Esp32 webserver https://fipsok.de/
 * - https://github.com/pmerlin/PMR-LED-Table/blob/master/tetrisGame.ino
 * - https://randomnerdtutorials.com/wifimanager-with-esp8266-autoconnect-custom-parameter-and-manage-your-ssid-and-password/ 
 * 
 */

#include "secrets.h"                    // rename the file example_secrets.h to secrets.h after cloning the project. More information in README.md
#include <LittleFS.h>
#include <Adafruit_GFX.h>               // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_NeoMatrix.h>         // https://github.com/adafruit/Adafruit_NeoMatrix
#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs: https://github.com/adafruit/Adafruit_NeoPixel
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "Base64.h"                    // copied from https://github.com/Xander-Electronics/Base64 
#include <DNSServer.h>
#include <WiFiManager.h>                //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <EEPROM.h>                     //from ESP8266 Arduino Core (automatically installed when ESP8266 was installed via Boardmanager)

// own libraries
#include "udplogger.h"
#include "ntp_client_plus.h"
#include "ledmatrix.h"
#include "tetris.h"
#include "snake.h"
#include "pong.h"


// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------

#define EEPROM_SIZE 30      // size of EEPROM to save persistent variables
#define ADR_NM_START_H 0
#define ADR_NM_END_H 4
#define ADR_NM_START_M 8
#define ADR_NM_END_M 12
#define ADR_BRIGHTNESS 16
#define ADR_MC_RED 20
#define ADR_MC_GREEN 22
#define ADR_MC_BLUE 24
#define ADR_STATE 26
#define ADR_NM_ACTIVATED 27
#define ADR_COLSHIFTSPEED 28
#define ADR_COLSHIFTACTIVE 29


#define NEOPIXELPIN 5       // pin to which the NeoPixels are attached
#define BUTTONPIN 14        // pin to which the button is attached
#define LEFT 1
#define RIGHT 2
#define LINE 10
#define RECT 5

#define PERIOD_HEARTBEAT 5000
#define PERIOD_ANIMATION 200
#define PERIOD_TETRIS 50
#define PERIOD_SNAKE 50
#define PERIOD_PONG 10
#define TIMEOUT_LEDDIRECT 5000
#define PERIOD_STATECHANGE 10000
#define PERIOD_NTPUPDATE 30000
#define PERIOD_TIMEVISUUPDATE 1000
#define PERIOD_MATRIXUPDATE 100
#define PERIOD_NIGHTMODECHECK 20000

#define SHORTPRESS 100
#define LONGPRESS 2000

#define CURRENT_LIMIT_LED 2500 // limit the total current sonsumed by LEDs (mA)

#define DEFAULT_SMOOTHING_FACTOR 0.5

// number of colors in colors array
#define NUM_COLORS 7

// own datatype for matrix movement (snake and spiral)
enum direction {right, left, up, down};

// width of the led matrix
#define WIDTH 11
// height of the led matrix
#define HEIGHT 11

// own datatype for state machine states
#define NUM_STATES 6
enum ClockState {st_clock, st_diclock, st_spiral, st_tetris, st_snake, st_pingpong};
const String stateNames[] = {"Clock", "DiClock", "Sprial", "Tetris", "Snake", "PingPong"};

// ports
const unsigned int localPort = 2390;
const unsigned int HTTPPort = 80;
const unsigned int logMulticastPort = 8123;
const unsigned int DNSPort = 53;

// ip addresses for multicast logging
IPAddress logMulticastIP = IPAddress(230, 120, 10, 2);

// ip addresses for Access Point
IPAddress IPAdress_AccessPoint(192,168,10,2);
IPAddress Gateway_AccessPoint(192,168,10,0);
IPAddress Subnetmask_AccessPoint(255,255,255,0);

// hostname
const String hostname = "wordclock";

// URL DNS server
const char WebserverURL[] = "www.wordclock.local";

int utcOffset = 60; // UTC offset in minutes

// ----------------------------------------------------------------------------------
//                                        GLOBAL VARIABLES
// ----------------------------------------------------------------------------------

// Webserver
ESP8266WebServer server(HTTPPort);

//DNS Server
DNSServer DnsServer;

// Wifi server. keep around to support resetting.
WiFiManager wifiManager;

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT+1, NEOPIXELPIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);


// seven predefined colors24bit (green, red, yellow, purple, orange, lightgreen, blue) 
const uint32_t colors24bit[NUM_COLORS] = {
  LEDMatrix::Color24bit(0, 255, 0),
  LEDMatrix::Color24bit(255, 0, 0),
  LEDMatrix::Color24bit(200, 200, 0),
  LEDMatrix::Color24bit(255, 0, 200),
  LEDMatrix::Color24bit(255, 128, 0), 
  LEDMatrix::Color24bit(0, 128, 0), 
  LEDMatrix::Color24bit(0, 0, 255) };

uint8_t brightness = 40;            // current brightness of leds
bool sprialDir = false;

// timestamp variables
long lastheartbeat = millis();      // time of last heartbeat sending
long lastStep = millis();           // time of last animation step
long lastLEDdirect = -TIMEOUT_LEDDIRECT; // time of last direct LED command (=> fall back to normal mode after timeout)
long lastStateChange = millis();    // time of last state change
long lastNTPUpdate = millis() - (PERIOD_NTPUPDATE-3000);  // time of last NTP update
long lastAnimationStep = millis();  // time of last Matrix update
long lastNightmodeCheck = millis()  - (PERIOD_NIGHTMODECHECK-3000); // time of last nightmode check
long buttonPressStart = 0;          // time of push button press start 
uint16_t behaviorUpdatePeriod = PERIOD_TIMEVISUUPDATE; // holdes the period in which the behavior should be updated

// Create necessary global objects
UDPLogger logger;
WiFiUDP NTPUDP;
NTPClientPlus ntp = NTPClientPlus(NTPUDP, "pool.ntp.org", utcOffset, true);
LEDMatrix ledmatrix = LEDMatrix(&matrix, brightness, &logger);
Tetris mytetris = Tetris(&ledmatrix, &logger);
Snake mysnake = Snake(&ledmatrix, &logger);
Pong mypong = Pong(&ledmatrix, &logger);

float filterFactor = DEFAULT_SMOOTHING_FACTOR;// stores smoothing factor for led transition
uint8_t currentState = st_clock;              // stores current state
bool stateAutoChange = false;                 // stores state of automatic state change
bool nightMode = false;                       // stores state of nightmode
bool nightModeActivated = true;               // stores if the function nightmode is activated (its not the state of nightmode)
bool ledOff = false;                          // stores state of led off
uint32_t maincolor_clock = colors24bit[2];    // color of the clock and digital clock
uint32_t maincolor_snake = colors24bit[1];    // color of the random snake animation
bool apmode = false;                          // stores if WiFi AP mode is active
bool dynColorShiftActive = false;              // stores if dynamic color shift is active
uint8_t dynColorShiftPhase = 0;               // stores the phase of the dynamic color shift
uint8_t dynColorShiftSpeed = 1;               // stores the speed of the dynamic color shift -> used to calc update period

// nightmode settings
uint8_t nightModeStartHour = 22;
uint8_t nightModeStartMin = 0;
uint8_t nightModeEndHour = 7;
uint8_t nightModeEndMin = 0;

// Watchdog counter to trigger restart if NTP update was not possible 30 times in a row (5min)
int watchdogCounter = 30;

bool waitForTimeAfterReboot = false; // wait for time update after reboot

// ----------------------------------------------------------------------------------
//                                        SETUP
// ----------------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.printf("\nSketchname: %s\nBuild: %s\n", (__FILE__), (__TIMESTAMP__));
  Serial.println();

  //Init EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // configure button pin as input
  pinMode(BUTTONPIN, INPUT_PULLUP);

  // setup Matrix LED functions
  ledmatrix.setupMatrix();
  ledmatrix.setCurrentLimit(CURRENT_LIMIT_LED);

  if(ESP.getResetReason().equals("Power On") || ESP.getResetReason().equals("External System")){
    // Turn on minutes leds (blue)
    ledmatrix.setMinIndicator(15, colors24bit[6]);
    ledmatrix.drawOnMatrixInstant();
  }


  /** Use WiFiMaanger for handling initial Wifi setup **/

  // Local intialization. Once its business is done, there is no need to keep it around


  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  // set custom ip for portal
  //wifiManager.setAPStaticIPConfig(IPAdress_AccessPoint, Gateway_AccessPoint, Subnetmask_AccessPoint);

  // set a custom hostname
  wifiManager.setHostname(hostname);
  
  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here "wordclockAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect(AP_SSID);

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP()); 

  if(ESP.getResetReason().equals("Power On") || ESP.getResetReason().equals("External System")){
    // Turn off minutes leds
    ledmatrix.setMinIndicator(15, 0);
    ledmatrix.drawOnMatrixInstant();
  }

   
  
  /** (alternative) Use directly STA/AP Mode of ESP8266   **/
  
  /* 
  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  //Set new hostname
  WiFi.hostname(hostname.c_str());
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  //wifi_station_set_hostname("esplamp");

  int timeoutcounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutcounter < 30) {
    ledmatrix.setMinIndicator(15, colors24bit[6]);
    ledmatrix.drawOnMatrixInstant();
    delay(250);
    ledmatrix.setMinIndicator(15, 0);
    ledmatrix.drawOnMatrixInstant();
    delay(250);
    Serial.print(".");
    timeoutcounter++;
  }

  // start request of program
  if (WiFi.status() == WL_CONNECTED) {      //Check WiFi connection status
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); 
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  
  } else {
    // no wifi found -> open access point
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(IPAdress_AccessPoint, Gateway_AccessPoint, Subnetmask_AccessPoint);
    WiFi.softAP(AP_SSID, AP_PASS);
    apmode = true;

    // start DNS Server
    DnsServer.setTTL(300);
    DnsServer.start(DNSPort, WebserverURL, IPAdress_AccessPoint);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }*/

  // init ESP8266 File manager (LittleFS)
  setupFS();

  // setup OTA
  setupOTA(hostname);

  server.on("/cmd", handleCommand); // process commands
  server.on("/data", handleDataRequest); // process datarequests
  server.on("/leddirect", HTTP_POST, handleLEDDirect); // Call the 'handleLEDDirect' function when a POST request is made to URI "/leddirect"
  server.begin();
  
  // create UDP Logger to send logging messages via UDP multicast
  logger = UDPLogger(WiFi.localIP(), logMulticastIP, logMulticastPort);
  logger.setName("Wordclock 2.0");
  logger.logString("Start program\n");
  delay(10);
  logger.logString("Sketchname: "+ String(__FILE__));
  delay(10);
  logger.logString("Build: " + String(__TIMESTAMP__));
  delay(10);
  logger.logString("IP: " + WiFi.localIP().toString());
  delay(10);
  logger.logString("Reset Reason: " + ESP.getResetReason());

  // setup NTP
  updateUTCOffsetFromTimezoneAPI(logger, ntp);
  ntp.setupNTPClient();
  logger.logString("NTP running");
  logger.logString("Time: " +  ntp.getFormattedTime());

  // load persistent variables from EEPROM
  loadMainColorFromEEPROM();
  loadCurrentStateFromEEPROM();
  loadNightmodeSettingsFromEEPROM();
  loadBrightnessSettingsFromEEPROM();
  loadColorShiftStateFromEEPROM();
  
  if(ESP.getResetReason().equals("Power On") || ESP.getResetReason().equals("External System")){
    // test quickly each LED
    for(int r = 0; r < HEIGHT; r++){
        for(int c = 0; c < WIDTH; c++){
        matrix.fillScreen(0);
        matrix.drawPixel(c, r, LEDMatrix::color24to16bit(colors24bit[2]));
        matrix.show();
        delay(10); 
        }
    }
    
    // clear Matrix
    matrix.fillScreen(0);
    matrix.show();
    delay(200);

    // display IP
    uint8_t address = WiFi.localIP()[3];
    ledmatrix.printChar(1, 0, 'I', maincolor_clock);
    ledmatrix.printChar(5, 0, 'P', maincolor_clock);
    ledmatrix.printNumber(0, 6, (address/100), maincolor_clock);
    ledmatrix.printNumber(4, 6, (address/10)%10, maincolor_clock);
    ledmatrix.printNumber(8, 6, address%10, maincolor_clock);
    ledmatrix.drawOnMatrixInstant();
    delay(2000);

    // clear matrix
    ledmatrix.gridFlush();
    ledmatrix.drawOnMatrixInstant();
  }
  else {
    waitForTimeAfterReboot = true;
  }

  // run the entry action for the initial state
  entryAction(currentState);
}


// ----------------------------------------------------------------------------------
//                                        LOOP
// ----------------------------------------------------------------------------------

void loop() {
  // handle OTA
  handleOTA();
  
  // handle Webserver
  server.handleClient();

  // send regularly heartbeat messages via UDP multicast
  if(millis() - lastheartbeat > PERIOD_HEARTBEAT){
    logger.logString("Heartbeat, state: " + stateNames[currentState] + ", FreeHeap: " + ESP.getFreeHeap() + ", HeapFrag: " + ESP.getHeapFragmentation() + ", MaxFreeBlock: " + ESP.getMaxFreeBlockSize() + "\n");
    lastheartbeat = millis();

    // Check wifi status (only if no apmode)
    if(!apmode && WiFi.status() != WL_CONNECTED){
      Serial.println("connection lost");
      ledmatrix.gridAddPixel(0, 5, colors24bit[1]);
      ledmatrix.drawOnMatrixInstant();
      delay(1000);
    }
  }

  // handle state behaviours (trigger loopCycles of different states depending on current state)
  if(!nightMode && !ledOff && (millis() - lastStep > behaviorUpdatePeriod) && (millis() - lastLEDdirect > TIMEOUT_LEDDIRECT)){
    updateStateBehavior(currentState);    
    lastStep = millis();
  }

  // Turn off LEDs if ledOff is true or nightmode is active
  if((ledOff || nightMode) && !waitForTimeAfterReboot){
    ledmatrix.gridFlush();
  }

  // periodically write colors to matrix
  if(millis() - lastAnimationStep > PERIOD_MATRIXUPDATE && !waitForTimeAfterReboot && (millis() - lastLEDdirect > TIMEOUT_LEDDIRECT)){
    ledmatrix.drawOnMatrixSmooth(filterFactor);
    lastAnimationStep = millis();
  }

  // handle button press
  handleButton();

  // handle state changes
  if(stateAutoChange && (millis() - lastStateChange > PERIOD_STATECHANGE) && !nightMode && !ledOff){
    // increment state variable and trigger state change
    stateChange((currentState + 1) % NUM_STATES, false);
    
    // save last automatic state change
    lastStateChange = millis();
  }

  // NTP time update
  if(millis() - lastNTPUpdate > PERIOD_NTPUPDATE){
    int res = ntp.updateNTP();
    if(res == 0){
      ntp.calcDate();
      logger.logString("NTP-Update successful");
      logger.logString("Time: " +  ntp.getFormattedTime());
      logger.logString("Date: " +  ntp.getFormattedDate());
      logger.logString("Day of Week (Mon=1, Sun=7): " +  String(ntp.getDayOfWeek()));
      logger.logString("Summertime: " + String(ntp.updateSWChange()));
      lastNTPUpdate = millis();
      watchdogCounter = 30;
      checkNightmode();
      if(waitForTimeAfterReboot && !nightMode){
        // update mode (e.g. write the current time onto the matrix) first time after reboot
        entryAction(currentState);
        updateStateBehavior(currentState);
        ledmatrix.drawOnMatrixInstant();
      }
      waitForTimeAfterReboot = false;
    }
    else if(res == -1){
      logger.logString("NTP-Update not successful. Reason: Timeout");
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }
    else if(res == 1){
      logger.logString("NTP-Update not successful. Reason: Too large time difference");
      logger.logString("Time: " +  ntp.getFormattedTime());
      logger.logString("Date: " +  ntp.getFormattedDate());
      logger.logString("Day of Week (Mon=1, Sun=7): " +  ntp.getDayOfWeek());
      logger.logString("Summertime: " + String(ntp.updateSWChange()));
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }
    else {
      logger.logString("NTP-Update not successful. Reason: NTP time not valid (<1970)");
      lastNTPUpdate += 10000;
      watchdogCounter--;
    }

    logger.logString("Watchdog Counter: " + String(watchdogCounter));
    if(watchdogCounter <= 0){
        logger.logString("Trigger restart due to watchdog...");
        delay(100);
        ESP.restart();
    }
    
  }

  // check if nightmode need to be activated
  if(millis() - lastNightmodeCheck > PERIOD_NIGHTMODECHECK && !waitForTimeAfterReboot){
    checkNightmode();
    lastNightmodeCheck = millis();
  }
 
}


// ----------------------------------------------------------------------------------
//                                        OTHER FUNCTIONS
// ----------------------------------------------------------------------------------

/**
 * @brief Update mode behaviour depending on current state
 */
void updateStateBehavior(uint8_t state){
  switch(state){
    // state clock
    case st_clock:
      {
        if(dynColorShiftActive){
          dynColorShiftPhase = (dynColorShiftPhase + 1) % 256;
          ledmatrix.setDynamicColorShiftPhase(dynColorShiftPhase);
          filterFactor = 1.0; // no smoothing
          behaviorUpdatePeriod = PERIOD_TIMEVISUUPDATE / dynColorShiftSpeed;
        } else {
          ledmatrix.setDynamicColorShiftPhase(-1);
          filterFactor = DEFAULT_SMOOTHING_FACTOR;
          behaviorUpdatePeriod = PERIOD_TIMEVISUUPDATE;
        }
        uint8_t hours = ntp.getHours24();
        uint8_t minutes = ntp.getMinutes();
        static uint8_t lastMinutes = 0;
        static String timeAsString = "";
        if(lastMinutes != minutes){
          timeAsString = timeToString(hours, minutes);
          lastMinutes = minutes;
        }
        showStringOnClock(timeAsString, maincolor_clock);
        drawMinuteIndicator(minutes, maincolor_clock);
      }
      break;
    // state diclock
    case st_diclock:
      {
        int hours = ntp.getHours24();
        int minutes = ntp.getMinutes();
        showDigitalClock(hours, minutes, maincolor_clock);
      }
      break;
    // state spiral
    case st_spiral:
      {
        int res = spiral(false, sprialDir, WIDTH-6);
        if(res && sprialDir == 0){
          // change spiral direction to closing (draw empty leds)
          sprialDir = 1;
          // init spiral with new spiral direction
          spiral(true, sprialDir, WIDTH-6);
          
        }else if(res && sprialDir == 1){
          // reset spiral direction to normal drawing leds
          sprialDir = 0;
          // init spiral with new spiral direction
          spiral(true, sprialDir, WIDTH-6);
        }
      }
      break;
    // state tetris
    case st_tetris:
      {
        if(stateAutoChange){
          randomtetris(false);
        }
        else{
          mytetris.loopCycle();
        }
      }
      break;
    // state snake
    case st_snake:
      {
        if(stateAutoChange){
          ledmatrix.gridFlush();
          int res = randomsnake(false, 8, maincolor_snake, -1);
          if(res){
            // init snake for next run
            randomsnake(true, 8, maincolor_snake, -1);
          }
        }
        else{
          mysnake.loopCycle();
        }
      }
      break;
    // state pingpong
    case st_pingpong:
      {
        mypong.loopCycle();
      }
      break;
  }
}

/**
 * @brief Check if nightmode should be activated
 * 
 */
void checkNightmode(){
  logger.logString("Check nightmode");
  int hours = ntp.getHours24();
  int minutes = ntp.getMinutes();
  
  nightMode = false; // Initial assumption

  // Convert all times to minutes for easier comparison
  int currentTimeInMinutes = hours * 60 + minutes;
  int startInMinutes = nightModeStartHour * 60 + nightModeStartMin;
  int endInMinutes = nightModeEndHour * 60 + nightModeEndMin;

  if (startInMinutes < endInMinutes && nightModeActivated) { // Same day scenario
      if (startInMinutes < currentTimeInMinutes && currentTimeInMinutes < endInMinutes) {
          nightMode = true;
          logger.logString("Nightmode active");
      }
  } else if (startInMinutes > endInMinutes && nightModeActivated) { // Overnight scenario
      if (currentTimeInMinutes >= startInMinutes || currentTimeInMinutes < endInMinutes) {
          nightMode = true;
          logger.logString("Nightmode active");
      }
  }
}

/**
 * @brief call entry action of given state
 * 
 * @param state 
 */
void entryAction(uint8_t state){
  filterFactor = DEFAULT_SMOOTHING_FACTOR;
  switch(state){
    case st_clock:
      behaviorUpdatePeriod = PERIOD_TIMEVISUUPDATE;
      break;
    case st_diclock:
      behaviorUpdatePeriod = PERIOD_TIMEVISUUPDATE;
      ledmatrix.setDynamicColorShiftPhase(-1); // disable dyn. color shift
      break;
    case st_spiral:
      behaviorUpdatePeriod = PERIOD_ANIMATION;
      ledmatrix.setDynamicColorShiftPhase(-1); // disable dyn. color shift
      // Init spiral with normal drawing mode
      sprialDir = 0;
      spiral(true, sprialDir, WIDTH-6);
      break;
    case st_tetris:
      ledmatrix.setDynamicColorShiftPhase(-1); // disable dyn. color shift
      filterFactor = 1.0; // no smoothing
      if(stateAutoChange){
        behaviorUpdatePeriod = PERIOD_ANIMATION;
        randomtetris(true);
      }
      else{
        behaviorUpdatePeriod = PERIOD_TETRIS;
        mytetris.ctrlStart();
      }
      break;
    case st_snake:
      ledmatrix.setDynamicColorShiftPhase(-1); // disable dyn. color shift
      if(stateAutoChange){
        behaviorUpdatePeriod = PERIOD_ANIMATION;
        randomsnake(true, 8, colors24bit[1], -1);
      }
      else{
        behaviorUpdatePeriod = PERIOD_SNAKE;
        filterFactor = 1.0; // no smoothing
        mysnake.initGame();
      }
      break;
    case st_pingpong:
      behaviorUpdatePeriod = PERIOD_PONG;
      ledmatrix.setDynamicColorShiftPhase(-1); // disable dyn. color shift
      if(stateAutoChange){
        mypong.initGame(2);
      }
      else{
        filterFactor = 1.0; // no smoothing
        mypong.initGame(1);
      }
      break;
  }
}

/**
 * @brief execute a state change to given newState
 * 
 * @param newState the new state to be changed to
 * @param persistant if true, the state will be saved to EEPROM
 */
void stateChange(uint8_t newState, bool persistant){
  if(ledOff){
    ledOff = false;
  }
  // first clear matrix
  ledmatrix.gridFlush();
  // set new state
  currentState = newState;
  entryAction(currentState);
  logger.logString("State change to: " + stateNames[currentState]);
  if(persistant){
    // save state to EEPROM
    EEPROM.write(ADR_STATE, currentState);
    EEPROM.commit();
  }
}

/**
 * @brief Handler for POST requests to /leddirect.
 * 
 * Allows the control of all LEDs from external source. 
 * It will overwrite the normal program for 5 seconds.
 * A 11x11 picture can be sent as base64 encoded string to be displayed on matrix.
 * 
 */
void handleLEDDirect() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = "POST data was:\n";
    /*logger.logString(message);
    delay(10);
    for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
      logger.logString(server.arg(i));
      delay(10);
    }*/
    if(server.args() == 1){
      String data = String(server.arg(0));
      int dataLength = data.length();
      //char byteArray[dataLength];
      //data.toCharArray(byteArray, dataLength);

      // base64 decoding
      char base64data[dataLength];
      data.toCharArray(base64data, dataLength);
      int base64dataLen = dataLength;
      int decodedLength = Base64.decodedLength(base64data, base64dataLen);
      char byteArray[decodedLength];
      Base64.decode(byteArray, base64data, base64dataLen);

      /*for(int i = 0; i < 10; i++){
        logger.logString(String((int)(byteArray[i])));
        delay(10);
      }*/


      for(int i = 0; i < dataLength; i += 4) {
        uint8_t red = byteArray[i]; // red
        uint8_t green = byteArray[i + 1]; // green
        uint8_t blue = byteArray[i + 2]; // blue
        ledmatrix.gridAddPixel((i/4) % WIDTH, (i/4) / HEIGHT, LEDMatrix::Color24bit(red, green, blue));
      }
      ledmatrix.drawOnMatrixInstant();

      lastLEDdirect = millis();
    }
    server.send(200, "text/plain", message);
  }
}

/**
 * @brief Check button commands
 * 
 */
void handleButton(){
  static bool lastButtonState = false;
  bool buttonPressed = !digitalRead(BUTTONPIN);
  // check rising edge
  if(buttonPressed == true && lastButtonState == false){
    // button press start
    logger.logString("Button press started");
    buttonPressStart = millis();
  }
  // check falling edge
  if(buttonPressed == false && lastButtonState == true){
    // button press ended
    if((millis() - buttonPressStart) > LONGPRESS){
      // longpress -> nightmode
      logger.logString("Button press ended - longpress");

      ledOff = true;
    }
    else if((millis() - buttonPressStart) > SHORTPRESS){
      // shortpress -> state change 
      logger.logString("Button press ended - shortpress");

      if(ledOff){
        ledOff = false;
      }else{
        stateChange((currentState + 1) % NUM_STATES, true);
      }
      
    }
  }
  lastButtonState = buttonPressed;
}

/**
 * @brief Set main color
 * 
 */
void setMainColor(uint8_t red, uint8_t green, uint8_t blue){
  maincolor_clock = LEDMatrix::Color24bit(red, green, blue);
  EEPROM.put(ADR_MC_RED, red);
  EEPROM.put(ADR_MC_GREEN, green);
  EEPROM.put(ADR_MC_BLUE, blue);
  EEPROM.commit();
}

/**
 * @brief Load maincolor from EEPROM
 * 
*/
void loadMainColorFromEEPROM(){
  uint8_t red = EEPROM.read(ADR_MC_RED);
  uint8_t green = EEPROM.read(ADR_MC_GREEN);
  uint8_t blue = EEPROM.read(ADR_MC_BLUE);
  if(int(red) + int(green) + int(blue) < 50){
    maincolor_clock = colors24bit[2];
  }else{
    maincolor_clock = LEDMatrix::Color24bit(red, green, blue);
  }
}

/**
 * @brief Load the current state from EEPROM
 * 
 */
void loadCurrentStateFromEEPROM(){
  currentState = EEPROM.read(ADR_STATE);
  if(currentState >= NUM_STATES){
    currentState = st_clock;
    EEPROM.write(ADR_STATE, currentState);
    EEPROM.commit();
  }
}

/**
 * @brief Load the nightmode settings from EEPROM
 */
void loadNightmodeSettingsFromEEPROM()
{
  nightModeStartHour = EEPROM.read(ADR_NM_START_H);
  nightModeStartMin = EEPROM.read(ADR_NM_START_M);
  nightModeEndHour = EEPROM.read(ADR_NM_END_H);
  nightModeEndMin = EEPROM.read(ADR_NM_END_M);
  nightModeActivated = EEPROM.read(ADR_NM_ACTIVATED);
  if(nightModeStartHour < 0 || nightModeStartHour > 23) nightModeStartHour = 22;
  if(nightModeStartMin < 0 || nightModeStartMin > 59) nightModeStartMin = 0;
  if(nightModeEndHour < 0 || nightModeEndHour > 23) nightModeEndHour = 7;
  if(nightModeEndMin < 0 || nightModeEndMin > 59) nightModeEndMin = 0;
  logger.logString("Nightmode activated: " + String(nightModeActivated));
  logger.logString("Nightmode starts at: " + String(nightModeStartHour) + ":" + String(nightModeStartMin));
  logger.logString("Nightmode ends at: " + String(nightModeEndHour) + ":" + String(nightModeEndMin));
}

/**
 * @brief Load the brightness settings from EEPROM
 *
 * lower limit is 10 so that the LEDs are not completely off
 */
void loadBrightnessSettingsFromEEPROM()
{
  brightness = EEPROM.read(ADR_BRIGHTNESS);
  if(brightness < 10) brightness = 10;
  logger.logString("Brightness: " + String(brightness));
  ledmatrix.setBrightness(brightness);
}

/**
 * @brief load the color shift speed from EEPROM
 *
 */
void loadColorShiftStateFromEEPROM()
{
  dynColorShiftSpeed = EEPROM.read(ADR_COLSHIFTSPEED);
  if (dynColorShiftSpeed == 0) dynColorShiftSpeed = 1;
  logger.logString("ColorShiftSpeed: " + String(dynColorShiftSpeed));
  dynColorShiftActive = EEPROM.read(ADR_COLSHIFTACTIVE);
  logger.logString("ColorShiftActive: " + String(dynColorShiftActive));
}

/**
 * @brief Handler for handling commands sent to "/cmd" url
 * 
 */
void handleCommand() {
  // receive command and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    String log_str = "Command received: " + server.argName(i) + " " + server.arg(i);
    logger.logString(log_str);
  }
  
  if (server.argName(0) == "led") // the parameter which was sent to this server is led color
  {
    String colorstr = server.arg(0) + "-";
    String redstr = split(colorstr, '-', 0);
    String greenstr= split(colorstr, '-', 1);
    String bluestr = split(colorstr, '-', 2);
    logger.logString(colorstr);
    logger.logString("r: " + String(redstr.toInt()));
    logger.logString("g: " + String(greenstr.toInt()));
    logger.logString("b: " + String(bluestr.toInt()));
    // set new main color
    setMainColor(redstr.toInt(), greenstr.toInt(), bluestr.toInt());
  }
  else if (server.argName(0) == "mode") // the parameter which was sent to this server is mode change
  {
    String modestr = server.arg(0);
    logger.logString("Mode change via Webserver to: " + modestr);
    // set current mode/state accordant sent mode
    if(modestr == "clock"){
      stateChange(st_clock, true);
    }
    else if(modestr == "diclock"){
      stateChange(st_diclock, true);
    }
    else if(modestr == "spiral"){
      stateChange(st_spiral, true);
    }
    else if(modestr == "tetris"){
      stateChange(st_tetris, true);
    }
    else if(modestr == "snake"){
      stateChange(st_snake, true);
    }
    else if(modestr == "pingpong"){
      stateChange(st_pingpong, true);
    } 
  }
  else if(server.argName(0) == "ledoff"){
    String modestr = server.arg(0);
    logger.logString("LED off change via Webserver to: " + modestr);
    if(modestr == "1") ledOff = true;
    else ledOff = false;
  }
  else if(server.argName(0) == "nightmodeactivated"){
    String modestr = server.arg(0);
    logger.logString("nightModeActivated change via Webserver to: " + modestr);
    if(modestr == "1") nightModeActivated = true;
    else nightModeActivated = false;
    EEPROM.write(ADR_NM_ACTIVATED, nightModeActivated);
    EEPROM.commit();
    checkNightmode();
  }
  else if(server.argName(0) == "setting"){
    String timestr = server.arg(0) + "-";
    logger.logString("Nightmode setting change via Webserver to: " + timestr);
    nightModeStartHour = split(timestr, '-', 0).toInt();
    nightModeStartMin = split(timestr, '-', 1).toInt();
    nightModeEndHour = split(timestr, '-', 2).toInt();
    nightModeEndMin = split(timestr, '-', 3).toInt();
    brightness = split(timestr, '-', 4).toInt();
    dynColorShiftSpeed = split(timestr, '-', 5).toInt();
    if(nightModeStartHour < 0 || nightModeStartHour > 23) nightModeStartHour = 22;
    if(nightModeStartMin < 0 || nightModeStartMin > 59) nightModeStartMin = 0;
    if(nightModeEndHour < 0 || nightModeEndHour > 23) nightModeEndHour = 7;
    if(nightModeEndMin < 0 || nightModeEndMin > 59) nightModeEndMin = 0;
    if(brightness < 10) brightness = 10;
    if(dynColorShiftSpeed == 0) dynColorShiftSpeed = 1;
    EEPROM.write(ADR_NM_START_H, nightModeStartHour);
    EEPROM.write(ADR_NM_START_M, nightModeStartMin);
    EEPROM.write(ADR_NM_END_H, nightModeEndHour);
    EEPROM.write(ADR_NM_END_M, nightModeEndMin);
    EEPROM.write(ADR_BRIGHTNESS, brightness);
    EEPROM.write(ADR_COLSHIFTSPEED, dynColorShiftSpeed);
    EEPROM.commit();
    logger.logString("Nightmode starts at: " + String(nightModeStartHour) + ":" + String(nightModeStartMin));
    logger.logString("Nightmode ends at: " + String(nightModeEndHour) + ":" + String(nightModeEndMin));
    logger.logString("Brightness: " + String(brightness));
    logger.logString("ColorShiftSpeed: " + String(dynColorShiftSpeed));
    ledmatrix.setBrightness(brightness);
    lastNightmodeCheck = millis()  - PERIOD_NIGHTMODECHECK;
  }
  else if (server.argName(0) == "resetwifi"){
    wifiManager.resetSettings();
    // run LED test.
    for(int r = 0; r < HEIGHT; r++){
      for(int c = 0; c < WIDTH; c++){
        matrix.fillScreen(0);
        matrix.drawPixel(c, r, LEDMatrix::color24to16bit(colors24bit[2]));
        matrix.show();
        delay(10); 
        }
    }
    
    // clear Matrix
    matrix.fillScreen(0);
    matrix.show();
    delay(200);
  }
  else if(server.argName(0) == "stateautochange"){
    String modestr = server.arg(0);
    logger.logString("stateAutoChange change via Webserver to: " + modestr);
    if(modestr == "1") stateAutoChange = true;
    else stateAutoChange = false;
  }
  else if(server.argName(0) == "tetris"){
    String cmdstr = server.arg(0);
    logger.logString("Tetris cmd via Webserver to: " + cmdstr);
    if(cmdstr == "up"){
      mytetris.ctrlUp();
    }
    else if(cmdstr == "left"){
      mytetris.ctrlLeft();
    }
    else if(cmdstr == "right"){
      mytetris.ctrlRight();
    }
    else if(cmdstr == "down"){
      mytetris.ctrlDown();
    }
    else if(cmdstr == "play"){
      mytetris.ctrlStart();
    }
    else if(cmdstr == "pause"){
      mytetris.ctrlPlayPause();
    }
  }
  else if(server.argName(0) == "snake"){
    String cmdstr = server.arg(0);
    logger.logString("Snake cmd via Webserver to: " + cmdstr);
    if(cmdstr == "up"){
      mysnake.ctrlUp();
    }
    else if(cmdstr == "left"){
      mysnake.ctrlLeft();
    }
    else if(cmdstr == "right"){
      mysnake.ctrlRight();
    }
    else if(cmdstr == "down"){
      mysnake.ctrlDown();
    }
    else if(cmdstr == "new"){
      mysnake.initGame();
    }
  }
  else if(server.argName(0) == "pong"){
    String cmdstr = server.arg(0);
    logger.logString("Pong cmd via Webserver to: " + cmdstr);
    if(cmdstr == "up"){
      mypong.ctrlUp(1);
    }
    else if(cmdstr == "down"){
      mypong.ctrlDown(1);
    }
    else if(cmdstr == "new"){
      mypong.initGame(1);
    }
  }
  else if(server.argName(0) == "reboot"){
    logger.logString("Reboot via Webserver");
    server.send(204, "text/plain", "No Content"); // this page doesn't send back content --> 204
    delay(1000);
    ESP.restart();
  }
  else if(server.argName(0) == "colorshift"){
    Serial.println("ColorShift change via Webserver");
    String str = server.arg(0);
    if(str == "1") dynColorShiftActive = true;
    else dynColorShiftActive = false;
    EEPROM.write(ADR_COLSHIFTACTIVE, dynColorShiftActive);
    EEPROM.commit();
  }
  server.send(204, "text/plain", "No Content"); // this page doesn't send back content --> 204
}

/**
 * @brief Splits a string at given character and return specified element
 * 
 * @param s string to split
 * @param parser separating character
 * @param index index of the element to return
 * @return String 
 */
String split(String s, char parser, int index) {
  String rs="";
  int parserCnt=0;
  int rFromIndex=0, rToIndex=-1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex+1;
    rToIndex = s.indexOf(parser,rFromIndex);
    if (index == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) return "";
      return s.substring(rFromIndex,rToIndex);
    } else parserCnt++;
  }
  return rs;
}

/**
 * @brief Handler for GET requests
 * 
 */
void handleDataRequest() {
  // receive data request and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
  }
  
  if (server.argName(0) == "key") // the parameter which was sent to this server is led color
  {
    String message = "{";
    String keystr = server.arg(0);
    if(keystr == "mode"){
      message += "\"mode\":\"" + stateNames[currentState] + "\"";
      message += ",";
      message += "\"modeid\":\"" + String(currentState) + "\"";
      message += ",";
      message += "\"stateAutoChange\":\"" + String(stateAutoChange) + "\"";
      message += ",";
      message += "\"ledoff\":\"" + String(ledOff) + "\"";
      message += ",";
      message += "\"nightModeActivated\":\"" + String(nightModeActivated) + "\"";
      message += ",";
      message += "\"nightModeStart\":\"" + leadingZero2Digit(nightModeStartHour) + "-" + leadingZero2Digit(nightModeStartMin) + "\"";
      message += ",";
      message += "\"nightModeEnd\":\"" + leadingZero2Digit(nightModeEndHour) + "-" + leadingZero2Digit(nightModeEndMin) + "\"";
      message += ",";
      message += "\"brightness\":\"" + String(brightness) + "\"";
      message += ",";
      message += "\"colorshift\":\"" + String(dynColorShiftActive) + "\"";
      message += ",";
      message += "\"colorshiftspeed\":\"" + String(dynColorShiftSpeed) + "\"";
    }
    message += "}";
    server.send(200, "application/json", message);
  }
}

/**
 * @brief Convert Integer to String with leading zero
 * 
 * @param value 
 * @return String 
 */
String leadingZero2Digit(int value){
  String msg = "";
  if(value < 10){
    msg = "0";
  }
  msg += String(value);
  return msg;
}
