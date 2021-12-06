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
    logmessage = _name + ": " + logmessage;
    Serial.println(logmessage);
    _Udp.beginPacketMulticast(_multicastAddr, _port, _interfaceAddr);
    logmessage.toCharArray(_packetBuffer, 100);
    _Udp.print(_packetBuffer);
    _Udp.endPacket();
}