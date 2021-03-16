#ifndef wifiserver_h
#define wifiserver_h

#include "Server.h"
#include "server_drv.h"

class WiFiClient;

class WiFiServer : public Server {
    private:
        ServerDrv serverdrv;
        uint16_t _port;
        int _sock_ser;
        bool _is_connected;
        uint8_t data[DATA_LENTH];
        int recvTimeout;

    public:
        WiFiServer(uint16_t);
        WiFiClient available(uint8_t* status = NULL);
        void begin();
        virtual size_t write(uint8_t b);
        virtual size_t write(const uint8_t *buf, size_t size);
        virtual void end();
        virtual void stop();
        virtual void close();
        int enableIPv6();
        int getIPv6Status();
        //uint8_t status();

        using Print::write;
};

#endif
