#include "udplogger.h"

UDPLogger::UDPLogger(){

}

UDPLogger::UDPLogger(IPAddress interfaceAddr, IPAddress multicastAddr, int port){
    _multicastAddr = multicastAddr;
    _port = port;
    _interfaceAddr = interfaceAddr;
    _name = "Log";
    _Udp.beginMulticast(_interfaceAddr, _multicastAddr, _port);
}

void UDPLogger::setName(String name){
    _name = name;
}

void UDPLogger::logString(const String& logmessage){
    // wait 5 milliseconds if last send was less than 5 milliseconds before 
    if(millis() < (_lastSend + 5)){
        delay(5);
    }
    // Fester Puffer statt String-Verkettung, um Heap-Fragmentierung zu vermeiden
    // (identisches Format und identische Kuerzung bei 100 Zeichen wie vorher)
    snprintf(_packetBuffer, sizeof(_packetBuffer), "%s: %s", _name.c_str(), logmessage.c_str());
    Serial.println(_packetBuffer);
    _Udp.beginPacketMulticast(_multicastAddr, _port, _interfaceAddr);
    _Udp.print(_packetBuffer);
    _Udp.endPacket();
    _lastSend=millis();
}

void UDPLogger::logColor24bit(uint32_t color){
  uint8_t resultRed = color >> 16 & 0xff;
  uint8_t resultGreen = color >> 8 & 0xff;
  uint8_t resultBlue = color & 0xff;
  logString(String(resultRed) + ", " + String(resultGreen) + ", " + String(resultBlue));
}
