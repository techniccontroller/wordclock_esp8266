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

void UDPLogger::logString(String logmessage){
    // wait 5 milliseconds if last send was less than 5 milliseconds before 
    if(millis() < (_lastSend + 5)){
        delay(5);
    }
    logmessage = _name + ": " + logmessage;
    Serial.println(logmessage);
    _Udp.beginPacketMulticast(_multicastAddr, _port, _interfaceAddr);
    logmessage.toCharArray(_packetBuffer, 100);
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