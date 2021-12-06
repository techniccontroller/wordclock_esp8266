#ifndef udplogger_h
#define udplogger_h

#include <Arduino.h>
#include <WiFiUdp.h>


class UDPLogger{

    public:
        UDPLogger();
        UDPLogger(IPAddress interfaceAddr, IPAddress multicastAddr, int port);
        void setName(String name);
        void logString(String logmessage);
    private:
        String _name;
        IPAddress _multicastAddr;
        IPAddress _interfaceAddr;
        int _port;
        WiFiUDP _Udp;
        char _packetBuffer[100];

};

#endif