/**
 * SmartDeskLamp - SmartDeskLamp
 * 
 * created by techniccontroller 04.11.2021
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
//#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include "udplogger.h"

#define NEOPIXELPIN 5       // pin to which the NeoPixels are attached
#define NUMPIXELS 125       // number of pixels attached to Attiny85
#define BUTTONPIN 14        // pin to which the button is attached
#define LEFT 1
#define RIGHT 2
#define LINE 10
#define RECT 5
// own datatype for matrix movement (snake and spiral)
enum direction {right, left, up, down};

// width of the led matrix
const int width = 11;
// height of the led matrix
const int height = 11;




ESP8266WebServer server(80);  // serve webserver on port 80

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(width, height+1, NEOPIXELPIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG,
  NEO_GRB            + NEO_KHZ800);


// six predefined colors (red, yellow, purple, orange, green, blue) 
const uint16_t colors[] = {
  matrix.Color(0, 0, 0),
  matrix.Color(255, 0, 0),
  matrix.Color(200, 200, 0),
  matrix.Color(255, 0, 200),
  matrix.Color(255, 128, 0), 
  matrix.Color(0, 128, 0), 
  matrix.Color(0, 0, 255) };

uint8_t brightness = 40;     // current brightness of leds
bool sprialDir = false;
long lastheartbeat = millis();
long lastStep = millis();
IPAddress logMulticastIP = IPAddress(230, 120, 10, 2);
int logMulticastPort = 8123;
UDPLogger logger;


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
  setupMatrix();

  delay(250);
  setMinIndicator(15, colors[6]);
  delay(1000);
  setMinIndicator(15, colors[0]);

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  
  // We start by connecting to a WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  wifi_station_set_hostname("esplamp");

  int timeoutcounter = 0;
  while (WiFi.status() != WL_CONNECTED && timeoutcounter < 30) {
    delay(250);
    setMinIndicator(15, colors[6]);
    delay(250);
    setMinIndicator(15, colors[6]);
    Serial.print(".");
    timeoutcounter++;
  }

  // start request of program
  if (WiFi.status() == WL_CONNECTED) {      //Check WiFi connection status
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); 
  
  } else {

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID + WiFi.macAddress(), AP_PASS);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
  }

  // init ESP8266 File manager
  spiffs();
  server.begin();

  // setup OTA
  setupOTA();

  server.on("/c.php", handleCommand); // process commands

  logger = UDPLogger(WiFi.localIP(), logMulticastIP, logMulticastPort);
  logger.setName(WiFi.localIP().toString());
  logger.logString("Start program\n");

  for(int r = 0; r < height; r++){
    for(int c = 0; c < width; c++){
      matrix.fillScreen(0);
      matrix.drawPixel(c, r, colors[2]);
      matrix.show();
      delay(50); 
    }
  }
  spiral(true, sprialDir);
  matrix.show();
  delay(200);
}

void loop() {
  // handle OTA
  handleOTA();
  
  // handle Webserver
  server.handleClient();

  if(millis() - lastheartbeat > 1000){
    logger.logString("Heartbeat\n");
    lastheartbeat = millis();
  }

  if(millis() - lastStep > 100){
    int res = spiral(false, sprialDir);
    if(res){
      sprialDir = !sprialDir;
      spiral(true, sprialDir);
    }
    matrix.show();
    lastStep = millis();
  }
  
}


void handleCommand() {
  // receive command and handle accordingly
  for (uint8_t i = 0; i < server.args(); i++) {
    Serial.print(server.argName(i));
    Serial.print(F(": "));
    Serial.println(server.arg(i));
  }
  
  if (server.argName(0) == "led") // the parameter which was sent to this server
  {
    String colorstr = server.arg(0);
    String redstr = split(colorstr, '-', 0);
    String greenstr= split(colorstr, '-', 1);
    String bluestr = split(colorstr, '-', 2);
    logger.logString(colorstr);
    logger.logString("r: " + String(redstr.toInt()));
    logger.logString("g: " + String(greenstr.toInt()));
    logger.logString("b: " + String(bluestr.toInt()));
    setMinIndicator(15, matrix.Color(redstr.toInt(), greenstr.toInt(), bluestr.toInt()));
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
