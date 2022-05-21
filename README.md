# Wordclock 2.0

Wordclock 2.0 with ESP8266 and NTP time

More details on my website: https://techniccontroller.de/word-clock-with-wifi-and-neopixel/


**Features:**
- 6 modes (Clock, Digital Clock, SPIRAL animation, TETRIS, SNAKE, PONG)
- time update via NTP server
- automatic summer/wintertime change
- easy WIFI setup with WifiManager
- configurable color
- configurable night mode (start and end time)
- configurable brightness
- automatic mode change
- webserver interface for configuration and control
- physical button to change mode or enable night mode without webserver
- automatic current limiting of LEDs

## Pictures of clock
![modes_images2](https://user-images.githubusercontent.com/36072504/156947689-dd90874d-a887-4254-bede-4947152d85c1.png)

## Screenshots of webserver UI
![screenshots_UI](https://user-images.githubusercontent.com/36072504/158478447-d828e460-d4eb-489e-981e-216e08d4b129.png)

## Quickstart

1. Clone the project into the sketch folder of the Arduino IDE, 
2. Install the additional libraries and flash it to the ESP8266 as usual (See section *Upload program to ESP8266* below). 
3. The implemented WiFiManager helps you to set up a WiFi connection with your home WiFi -> on the first startup it will create a WiFi access point named "WordclockAP". Connect your phone to this access point and follow the steps which will be shown to you. 
4. After a successful WiFi setup, open the browser and enter the IP address of your ESP8266 to access the interface of the webserver. 
5. Here you can then upload all files located in the folder "data". Please make sure all icons stay in the folder "icons" also on the webserver.


<img src="https://techniccontroller.de/wp-content/uploads/filemanager1-1.png" height="300px" /> <img src="https://techniccontroller.de/wp-content/uploads/filemanager2-1.png" height="300px" /> <img src="https://techniccontroller.de/wp-content/uploads/filemanager3-1.png" height="300px" />

## Install needed Libraries

Please download all these libraries as ZIP from GitHub, and extract them in the *libraries* folder of your Sketchbook location (see **File -> Preferences**):

- https://github.com/adafruit/Adafruit-GFX-Library
- https://github.com/adafruit/Adafruit_NeoMatrix
- https://github.com/adafruit/Adafruit_NeoPixel
- https://github.com/Xander-Electronics/Base64
- https://github.com/tzapu/WiFiManager
- https://github.com/esp8266/Arduino/tree/master/libraries/EEPROM
- https://github.com/adafruit/Adafruit_BusIO

folder structure should look like this:

```
MySketchbookLocation 
│
└───libraries
│   └───Adafruit-GFX-Library
│   └───Adafruit_NeoMatrix
│   └───Adafruit_NeoPixel
│   └───Base64
│   └───WiFiManager
│   └───EEPROM
│   └───Adafruit_BusIO
│   
└───wordclock_esp8266
    │   wordclock_esp8266.ino
    │   (...)
    |
    └───data
        │   (...)
        |
        └─── icons
        
        
```


## Upload program to ESP8266 with Arduino IDE

#### STEP1: Installation of Arduino IDE
First, the latest version of the Arduino IDE needs to be downloaded and installed from here.

#### STEP2: Installation of ESP8266 Arduino Core
To program the ESP8266 with the Arduino IDE, you need to install the board information first in Arduino IDE. To do that follow the following instructions:

- Start Arduino and open the File -> Preferences window.

- Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into the Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
![image](https://user-images.githubusercontent.com/36072504/169649790-1b85660e-8c7d-4dfe-a63a-5dfd9862a5de.png)

- Open Boards Manager from Tools > Board menu and search for "esp8266".

- Click the install button.

- Don’t forget to select your ESP8266 board from Tools > Board menu after installation (e.g NodeMCU 1.0)
![image](https://user-images.githubusercontent.com/36072504/169649801-898c4819-9145-45c5-b65b-52f2689ab646.png)

#### STEP3: Upload a program to ESP8266

- Open wordclock_esp8266.ino in Arduino IDE
- Connect ESP8266 board with Computer
- Select right serial Port in Tools -> Port
- Click on the upload button in the Arduino IDE to upload the program to the ESP8266 Module.     
![image](https://user-images.githubusercontent.com/36072504/169649810-1fda75c2-5f4d-4d71-98fe-30985d82f7f5.png)
