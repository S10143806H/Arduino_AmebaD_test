/*
  WiFiServer.cpp - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "WiFiServer.h"

#include <string.h>

#include "WiFi.h"
#include "WiFiClient.h"
#include "server_drv.h"

WiFiServer::WiFiServer(uint16_t port) {
    _port = port;
}

void WiFiServer::begin() {
    _sock_ser = serverdrv.startServer(_port);
}

WiFiClient WiFiServer::available(uint8_t* status) {
    int client_fd = (int)(status);
    client_fd = serverdrv.getAvailable(_sock_ser);
    return WiFiClient(client_fd);
}

size_t WiFiServer::write(uint8_t b) {
    return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t* buf, size_t size) {
    if (_sock_ser < 0) {
        setWriteError();
        return 0;
    }
    if (size == 0) {
        setWriteError();
        return 0;
    }

    if (!serverdrv.sendData(_sock_ser, buf, size)) {
        setWriteError();
        return 0;
    }
    return size;
}

void WiFiServer::stop() {
    if (_sock_ser < 0) {
        return;
    }
    serverdrv.stopSocket(_sock_ser);
    _is_connected = false;
    _sock_ser = -1;
}

void WiFiServer::end() {
   stop();
}

void WiFiServer::close() {
   stop();
}


int WiFiServer::enableIPv6()
{
    return serverdrv.enableIPv6();
}

int WiFiServer::getIPv6Status()
{
    return serverdrv.getIPv6Status();
}

#if 0
uint8_t WiFiServer::status() {
    return ServerDrv::getServerState(0);
}


size_t WiFiServer::write(uint8_t b)
{
    return write(&b, 1);
}

size_t WiFiServer::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;

    for (int sock = 0; sock < MAX_SOCK_NUM; sock++) {
        if (WiFiClass::_server_port[sock] != 0) {
            WiFiClient client(sock);
            if ((WiFiClass::_server_port[sock] == _port) && (client.status() == ESTABLISHED)) {
                n += client.write(buffer, size);
            }
        }
    }
    return n;
}
#endif
