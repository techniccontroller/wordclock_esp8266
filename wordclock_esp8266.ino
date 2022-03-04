/**
 * Wordclock 2.0 - Wordclock with ESP8266 and NTP time
 * 
 * created by techniccontroller 04.12.2021
 * 
 * components:
 * - ESP8266 (ESP-01)
 * - Neopixelstrip
 *  
 * 
 * with code parts from:
 * - Adafruit NeoPixel strandtest.ino, https://github.com/adafruit/Adafruit_NeoPixel/blob/master/examples/strandtest/strandtest.ino
 * - Esp8266 und Esp32 https://fipsok.de/
 */

#include "secrets.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
//#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <Base64_.h>                    // https://github.com/Xander-Electronics/Base64
#include "udplogger.h"
#include "ntp_client_plus.h"
#include "ledmatrix.h"
#include "tetris.h"


// ----------------------------------------------------------------------------------
//                                        CONSTANTS
// ----------------------------------------------------------------------------------

#define NEOPIXELPIN 5       // pin to which the NeoPixels are attached
#define NUMPIXELS 125       // number of pixels attached to Attiny85
#define BUTTONPIN 14        // pin to which the button is attached
#define LEFT 1
#define RIGHT 2
#define LINE 10
#define RECT 5

#define PERIOD_HEARTBEAT 1000
#define PERIOD_ANIMATION 200
#define PERIOD_TETRIS 50
#define TIMEOUT_LEDDIRECT 5000
#define PERIOD_STATECHANGE 10000
#define PERIOD_NTPUPDATE 30000
#define PERIOD_TIMEVISUUPDATE 1000
#define PERIOD_MATRIXUPDATE 100

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
const uint16_t PERIODS[NUM_STATES] = {PERIOD_TIMEVISUUPDATE, 
                                      PERIOD_TIMEVISUUPDATE, 
                                      PERIOD_ANIMATION,
                                      PERIOD_TETRIS, 
                                      PERIOD_ANIMATION,  
                                      PERIOD_ANIMATION};

// ports
const unsigned int localPort = 2390;
const unsigned int HTTPPort = 80;
const unsigned int logMulticastPort = 8123;

// ip addresses
IPAddress logMulticastIP = IPAddress(230, 120, 10, 2);
// hostname
String hostname = "wordclock";

// ----------------------------------------------------------------------------------
//                                        GLOBAL VARIABLES
// ----------------------------------------------------------------------------------

// Webserver
ESP8266WebServer server(HTTPPort);

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(WIDTH, HEIGHT+1, NEOPIXELPIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);


// seven predefined colors24bit (black, red, yellow, purple, orange, green, blue) 
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
long lastLEDdirect = 0;             // time of last direct LED command (=> fall back to normal mode after timeout)
long lastStateChange = millis();    // time of last state change
long lastNTPUpdate = millis();      // time of last NTP update
long lastAnimationStep = millis();       // time of last Matrix update
UDPLogger logger;
uint8_t currentState = st_clock;
WiFiUDP NTPUDP;
NTPClientPlus ntp = NTPClientPlus(NTPUDP, "pool.ntp.org", 1, true);
float filterFactor = 0.5;
LEDMatrix ledmatrix = LEDMatrix(&matrix, brightness, &logger);
Tetris mytetris = Tetris(&ledmatrix, &logger);

bool stateAutoChange = false;
bool nightMode = false;
uint32_t maincolor_clock = colors24bit[2];
uint32_t maincolor_snake = colors24bit[1];


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

  // button pin as input
  pinMode(BUTTONPIN, INPUT);

  // setup Matrix LED functions
  ledmatrix.setupMatrix();

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
    ledmatrix.setMinIndicator(0, colors24bit[6]);
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
  
  } else {
    // no wifi found -> open access point
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID + WiFi.macAddress(), AP_PASS);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }

  // init ESP8266 File manager
  spiffs();

  // setup OTA
  setupOTA();

  server.on("/cmd", handleCommand); // process commands
  server.on("/data", handleDataRequest); // process datarequests
  //server.on("/ledvideo", HTTP_POST, handleLEDVideo); // Call the 'handleLEDVideo' function when a POST request is made to URI "/ledvideo"
  //server.on("/leddirect", HTTP_POST, handleLEDDirect); // Call the 'handleLEDDirect' function when a POST request is made to URI "/leddirect"
  server.begin();
  
  logger = UDPLogger(WiFi.localIP(), logMulticastIP, logMulticastPort);
  logger.setName(WiFi.localIP().toString());
  logger.logString("Start program\n");
  logger.logString("Sketchname: "+ String(__FILE__) +"; Build: " + String(__TIMESTAMP__) + "");

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

  // setup NTP
  ntp.setupNTPClient();
  logger.logString("NTP running");
  logger.logString("Time: " +  ntp.getFormattedTime());

  // show the current time for short time in words
  int hours = ntp.getHours24();
  int minutes = ntp.getMinutes();
  String timeMessage = timeToString(hours, minutes);
  showStringOnClock(timeMessage, maincolor_clock);
  drawMinuteIndicator(minutes, maincolor_clock);
  ledmatrix.drawOnMatrixSmooth(filterFactor);
  delay(1000);


  // init all animation modes
  // init snake
  snake(true, 8, colors24bit[1], -1);
  // init spiral
  spiral(true, sprialDir, WIDTH-6);

  // show countdown
  /*for(int i = 9; i > 0; i--){
    logger.logString("DiTest: " + String(i));
    Serial.println("DiTest: " + String(i));
    ledmatrix.gridFlush();
    printNumber(4, 3, i, maincolor_clock);
    ledmatrix.drawOnMatrixInstant();
    delay(1000);
  }*/

  
}

void logColor(uint32_t color){
  uint8_t resultRed = color >> 16 & 0xff;
  uint8_t resultGreen = color >> 8 & 0xff;
  uint8_t resultBlue = color & 0xff;
  logger.logString(String(resultRed) + ", " + String(resultGreen) + ", " + String(resultBlue));
}


// ----------------------------------------------------------------------------------
//                                        LOOP
// ----------------------------------------------------------------------------------

void loop() {
  // handle OTA
  handleOTA();
  
  // handle Webserver
  server.handleClient();

  if(millis() - lastheartbeat > PERIOD_HEARTBEAT){
    logger.logString("Heartbeat, state: " + stateNames[currentState] + "\n");
    lastheartbeat = millis();
  }
  int res = 0;
  if(!nightMode && (millis() - lastStep > PERIODS[currentState]) && (millis() - lastLEDdirect > TIMEOUT_LEDDIRECT)){
    switch(currentState){
      // state clock
      case st_clock:
        {
          int hours = ntp.getHours24();
          int minutes = ntp.getMinutes();
          showStringOnClock(timeToString(hours, minutes), maincolor_clock);
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
          res = spiral(false, sprialDir, WIDTH-6);
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
          mytetris.loopCycle();
        }
        break;
      // state snake
      case st_snake:
        {
          ledmatrix.gridFlush();
          res = snake(false, 8, colors24bit[1], -1);
          if(res){
            // init snake for next run
            snake(true, 8, colors24bit[1], -1);
          }
        }
        break;
      // state pingpong
      case st_pingpong:
        {

        }
        break;
    }    
    
    lastStep = millis();
  }

  // periodically write colors to matrix
  if(!nightMode && (millis() - lastAnimationStep > PERIOD_MATRIXUPDATE)){
    ledmatrix.drawOnMatrixSmooth(filterFactor);
    lastAnimationStep = millis();
  }


  // handle state changes
  if(stateAutoChange && (millis() - lastStateChange > PERIOD_STATECHANGE) && !nightMode){
    // increment state variable and trigger state change
    stateChange((currentState + 1) % NUM_STATES);
    
    // save last automatic state change
    lastStateChange = millis();
  }

  // NTP time update
  if(millis() - lastNTPUpdate > PERIOD_NTPUPDATE){
    if(ntp.updateNTP()){
      logger.logString("NTP-Update successful");
      Serial.println("NTP-Update successful");
    }
    else{
      logger.logString("NTP-Update not successful");
      Serial.println("NTP-Update not successful");
    }
    lastNTPUpdate = millis();
  }
  
}


// ----------------------------------------------------------------------------------
//                                        OTHER FUNCTIONS
// ----------------------------------------------------------------------------------

/**
 * @brief call entry action of given state
 * 
 * @param state 
 */
void entryAction(uint8_t state){
  filterFactor = 0.5;
  switch(state){
    case st_spiral:
      // Init spiral with normal drawing mode
      sprialDir = 0;
      spiral(true, sprialDir, WIDTH-6);
      break;
    case st_tetris:
      filterFactor = 1.0;
      mytetris.onTetrisstartChange(true);
      delay(110);
      mytetris.onPlayChange(true);
      break;
  }
}

/**
 * @brief execute a state change to given newState
 * 
 * @param newState the new state to be changed to
 */
void stateChange(uint8_t newState){
  if(nightMode){
    // deactivate Nightmode
    setNightmode(false);
  }
  // first clear matrix
  ledmatrix.gridFlush();
  // set new state
  currentState = newState;
  entryAction(currentState);
  logger.logString("State change to: " + stateNames[currentState]);
  delay(5);
  logger.logString("FreeMemory=" + String(ESP.getFreeHeap()));
}


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
        matrix.drawPixel((i/4) % 11, (i/4) / 11, matrix.Color(red, green, blue));
      }
      matrix.show();

      lastLEDdirect = millis();


      

    }
    
    server.send(200, "text/plain", message);
  }
}

/**
 * @brief handler for handling commands sent to "/cmd" url
 * 
 */
void handleCommand() {
  // receive command and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
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
    maincolor_clock = LEDMatrix::Color24bit(redstr.toInt(), greenstr.toInt(), bluestr.toInt());
  }
  else if (server.argName(0) == "mode") // the parameter which was sent to this server is mode change
  {
    String modestr = server.arg(0);
    logger.logString("Mode change via Webserver to: " + modestr);
    // set current mode/state accordant sent mode
    if(modestr == "clock"){
      stateChange(st_clock);
    }
    else if(modestr == "diclock"){
      stateChange(st_diclock);
    }
    else if(modestr == "spiral"){
      stateChange(st_spiral);
    }
    else if(modestr == "tetris"){
      stateChange(st_tetris);
    }
    else if(modestr == "snake"){
      stateChange(st_snake);
    }
    else if(modestr == "pingpong"){
      stateChange(st_pingpong);
    } 
  }
  else if(server.argName(0) == "nightmode"){
    String modestr = server.arg(0);
    logger.logString("Nightmode change via Webserver to: " + modestr);
    if(modestr == "1") setNightmode(true);
    else setNightmode(false);
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
      mytetris.onHochChange(true);
    }
    else if(cmdstr == "left"){
      mytetris.onLinksChange(true);
    }
    else if(cmdstr == "right"){
      mytetris.onRechtsChange(true);
    }
    else if(cmdstr == "down"){
      mytetris.onRunterChange(true);
    }
    else if(cmdstr == "play"){
      mytetris.onPlayChange(true);
    }
    else if(cmdstr == "pause"){
      mytetris.onPauseChange(true);
    }
  }
  else if(server.argName(0) == "snake"){
    String cmdstr = server.arg(0);
    logger.logString("Snake cmd via Webserver to: " + cmdstr);
  }
  server.send(204, "text/plain", "No Content"); // this page doesn't send back content --> 204
}

String split(String s, char parser, int index) {
  String rs="";
  int parserIndex = index;
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
      message += "\"nightMode\":\"" + String(nightMode) + "\"";
    }
    message += "}";
    server.send(200, "application/json", message);
  }
}

void setNightmode(bool on){
  ledmatrix.gridFlush();
  ledmatrix.drawOnMatrixInstant();
  nightMode = on;
}
