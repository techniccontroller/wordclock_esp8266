/**
 * @file udplogger.h
 * @author techniccontroller (mail[at]techniccontroller.com)
 * @brief Class for sending logging Strings as multicast messages 
 * @version 0.1
 * @date 2022-03-21
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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
        void logColor24bit(uint32_t color);
    private:
        String _name;
        IPAddress _multicastAddr;
        IPAddress _interfaceAddr;
        int _port;
        WiFiUDP _Udp;
        char _packetBuffer[100];
        unsigned long _lastSend;
};

#endif