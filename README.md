# Wordclock 2.0
![compile esp8266 workflow](https://github.com/techniccontroller/wordclock_esp8266/actions/workflows/compile_esp8266.yml/badge.svg?branch=main)

A modern Wi-Fi Word Clock powered by the ESP8266 and synchronized via NTP (Network Time Protocol). 
Displays time in words with support for multiple languages and colorful NeoPixel LED effects.
Additional gaming modes, such as PONG, SNAKE, and TETRIS, can be played via an interactive Web UI.

**Project Details and Guide:**

Full tutorial and build instructions on https://techniccontroller.com/word-clock-with-wifi-and-neopixel/ 

## Features
- 6 modes
  - Word Clock
  - Digital Clock
  - SPIRAL animation
  - TETRIS (playable via web interface)
  - SNAKE (playable via web interface)
  - PONG (playable via web interface)
- Interactive Web-Based Games: Control PONG, TETRIS, and SNAKE directly through the built-in web UI
- Real-time clock synchronized over Wi-Fi using NTP
- Automatic daylight saving time (summer/winter) switching
- Automatic timezone detection
- Easy Wi-Fi setup with WiFiManager
- Configurable color themes
- Customizable night mode (start/end time)
- Adjustable brightness settings
- Automatic mode rotation
- Web interface for configuration and control
- Physical button for quick mode change or night mode toggle
- Intelligent current limiting of LEDs
- Dynamic color shifting mode 

## Supported Languages

The WordClock currently supports the following languages:
- **German (default)**
- **English**
- **Italian**
- **French**
- **Swiss German**
- **Javanese**

**How to Change the Language**

To switch to another language:
1. Go to the `wordclockfunctions.ino_<language>` file (e.g., `wordclockfunctions.ino_english`)
2. Rename it to `wordclockfunctions.ino`
3. Replace the existing `wordclockfunctions.ino` file
4. Compile and upload the code to your ESP8266

Only one language file should be named wordclockfunctions.ino at any time for successful compilation.

Thank you to everyone who provided feedback on adding new languages and testing their accuracy — your efforts have been invaluable in making this project truly inclusive and reliable!



## Pictures of clock
![modes_images2](https://user-images.githubusercontent.com/36072504/156947689-dd90874d-a887-4254-bede-4947152d85c1.png)

## Screenshots of webserver UI
![screenshots_UI](https://user-images.githubusercontent.com/36072504/158478447-d828e460-d4eb-489e-981e-216e08d4b129.png)

## Quickstart

1. Clone the project into the sketch folder of the Arduino IDE, 
2. Rename the file "example_secrets.h" to "secrets.h". You don't need to change anything in the file if you want to use the normal WiFi setup with WiFiManager (see section "Remark about the WiFi setup").
3. Install the additional libraries and upload the program to the ESP8266 as usual (See section [*Upload program to ESP8266*](https://github.com/techniccontroller/wordclock_esp8266/blob/main/README.md#upload-program-to-esp8266-with-arduino-ide) below). 
4. The implemented WiFiManager helps you to set up a WiFi connection with your home WiFi -> on the first startup it will create a WiFi access point named "WordclockAP". Connect your phone to this access point and follow the steps which will be shown to you. 
5. After a successful WiFi setup, open the browser and enter the IP address of your ESP8266 to access the interface of the webserver. 
6. Here you can upload all files located in the folder "data". Please make sure all icons stay in the folder "icons" also on the webserver.
    - Open **http://\<ip-address\>/fs.html** in a browser
    - Upload **fs.html**
    - Upload **style.css**
    - Upload **index.html**
    - Create a new folder **icons**
    - Upload all icons into this new folder **icons**


<img src="https://techniccontroller.com/wp-content/uploads/filemanager1-1.png" height="300px" /> <img src="https://techniccontroller.com/wp-content/uploads/filemanager2-1.png" height="300px" /> <img src="https://techniccontroller.com/wp-content/uploads/filemanager3-1.png" height="300px" />

## Special Branches 

We've got some interesting branches in this repo inspired by user feedback. These branches explore unique features and experimental ideas. Some will stay updated with the main branch's features.

- [**hour_animation**](https://github.com/techniccontroller/wordclock_esp8266/tree/hour_animation): This branch replaces the spiral animation with some custom pattern animation defined as x/y coordinate pattern including custom color for each letter. Also, this animation is show ones per hour.  
![compile esp8266 workflow](https://github.com/techniccontroller/wordclock_esp8266/actions/workflows/compile_esp8266.yml/badge.svg?branch=hour_animation)
- [**mode_seconds**](https://github.com/techniccontroller/wordclock_esp8266/tree/mode_seconds): This branch adds one additional mode to show the seconds as numbers on the clock. Thanks to [@Bram](https://github.com/BramWerbrouck)
![compile esp8266 workflow](https://github.com/techniccontroller/wordclock_esp8266/actions/workflows/compile_esp8266.yml/badge.svg?branch=mode_seconds)
- [**rgbw_leds**](https://github.com/techniccontroller/wordclock_esp8266/tree/rgbw_leds): This branch uses RGBW LEDs instead of RGB LEDs.  
![compile esp8266 workflow](https://github.com/techniccontroller/wordclock_esp8266/actions/workflows/compile_esp8266.yml/badge.svg?branch=rgbw_leds)
- [**static_background_pattern**](https://github.com/techniccontroller/wordclock_esp8266/tree/static_background_pattern): This branch allows to light up specific letters always during clock mode. E.G., to display some special words in another color.  
![compile esp8266 workflow](https://github.com/techniccontroller/wordclock_esp8266/actions/workflows/compile_esp8266.yml/badge.svg?branch=static_background_pattern)

## Install needed Libraries

Please download all these libraries as ZIP from GitHub, and extract them in the *libraries* folder of your Sketchbook location (see **File -> Preferences**):

- https://github.com/adafruit/Adafruit-GFX-Library
- https://github.com/adafruit/Adafruit_NeoMatrix
- https://github.com/adafruit/Adafruit_NeoPixel
- https://github.com/tzapu/WiFiManager
- https://github.com/adafruit/Adafruit_BusIO

You can als install these libraries via the library manager in the Arduino IDE.

The folder structure should look like this:

```
MySketchbookLocation 
│
└───libraries
│   └───Adafruit-GFX-Library
│   └───Adafruit_NeoMatrix
│   └───Adafruit_NeoPixel
│   └───WiFiManager
│   └───Adafruit_BusIO
│   
└───wordclock_esp8266
    │   wordclock_esp8266.ino
    │   (...)
    |
    └───data
        │   index.html
        |   (...)
        |
        └───icons 
```


## Upload program to ESP8266 with Arduino IDE

#### STEP1: Installation of Arduino IDE
First, the latest version of the Arduino IDE needs to be downloaded and installed from [here](https://www.arduino.cc/en/software).

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


## Remark about the WiFi setup

Regarding the WiFi setting, I have actually implemented two variants: 
1. By default the WifiManager is activated. That is, the word clock makes the first time its own WiFi (should be called "WordclockAP"). There you connect from a cell phone to `192.168.4.1`* and you can perform the configuration of the WiFi settings conveniently as with a SmartHome devices (Very elegant 😊)
2. Another (traditional) variant is to define the wifi credentials in the code (in secrets.h). 
    - For this you have to comment out lines 230 to 251 in the code of the file *wordclock_esp8266.ino* (add /\* before and \*/ after) 
    - and comment out lines 257 to 305 (remove /\* and \*/)
(* default IP provided by the WifiMAnager library.)

## Resetting the WiFi configuration

You can clear the stored WiFi credentials and restart the WiFi setup described above with these steps:
1. Open the settings panel in the web UI.
2. Enable 'Reset WiFi' slider.
3. Save settings.
4. LED test should be performed.
5. Disconnect and reconnect the power. WiFi credentials were removed. The setup should be restarted.
Resetting the wifi credentials does not delete uploaded files.

## Remark about Logging

The wordclock sends continuous log messages to the serial port and via multicast UDP. If you want to see these messages, you have to 

- open the serial monitor in the Arduino IDE (Tools -> Serial Monitor). The serial monitor must be set to 115200 baud.

OR

- run the following steps for the multicast UDP logging:

1. Starting situation: wordclock is connected to WLAN, a computer with installed Python (https://www.python.org/downloads/) is in the same local area network (WLAN or LAN doesn't matter).
3. Open the file **multicastUDP_receiver.py** in a text editor and in line 81 enter the IP address of the computer (not the wordclock!).
```python	
# ip address of network interface
MCAST_IF_IP = '192.168.0.7'
```
4. Execute the script with following command: 

```bash
python multicastUDP_receiver.py
```

5. Now you should see the log messages of the word clock (every 5 seconds a heartbeat message and the currently displayed time). 
If this is not the case, there could be a problem with the network settings of the computer, then recording is unfortunately not possible.

6. If special events (failed NTP update, reboot) occur, a section of the log is saved in a file called *log.txt*. 
In principle, the events are not critical and will occur from time to time, but should not be too frequent.
