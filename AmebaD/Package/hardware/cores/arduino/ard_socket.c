#include "ard_socket.h"

#include <lwip/netif.h>
#include <lwip/sockets.h>
#include <platform/platform_stdlib.h>
#include <platform_opts.h>

#define MAX_RECV_SIZE 1500
#define MAX_SEND_SIZE 256
#define UDP_SERVER_PORT 5002
#define TCP_SERVER_PORT 5003
#define UDP_SERVER_IP "fe80:0000:0000:0000:02e0:4cff:fe10:0068"
#define TCP_SERVER_IP "fe80:0000:0000:0000:02e0:4cff:fe10:0068"
#define TCP_SERVER_IP_TEST "2001:4860:4860::8888"

static int EXAMPLE_IPV6 = 0;

int start_server(uint16_t port, uint8_t protMode) {
    int _sock;
    int timeout;

    if (protMode == 0) {
        timeout = 3000;
        _sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        lwip_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    } else {
        timeout = 1000;
        _sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        lwip_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    }

    if (_sock < 0) {
        printf("\r\nERROR opening socket\r\n");
        return -1;
    }

    struct sockaddr_in localHost;
    memset(&localHost, 0, sizeof(localHost));

    localHost.sin_family = AF_INET;
    localHost.sin_port = htons(port);
    localHost.sin_addr.s_addr = INADDR_ANY;

    if (lwip_bind(_sock, ((struct sockaddr *)&localHost), sizeof(localHost)) < 0) {
        printf("\r\nERROR on binding\r\n");
        return -1;
    }

    return _sock;
}

int sock_listen(int sock, int max) {
    if (lwip_listen(sock, max) < 0) {
        printf("\r\nERROR on listening\r\n");
        return -1;
    }
    return 0;
}

int get_available(int sock) {
    int enable = 1;
    int timeout;
    int client_fd;
    int err;
    struct sockaddr_in cli_addr;

    socklen_t client = sizeof(cli_addr);

    do {
        client_fd = lwip_accept(sock, ((struct sockaddr *)&cli_addr), &client);
        if (client_fd < 0) {
            err = get_sock_errno(sock);
            if (err != EAGAIN) {
                break;
            }
        }
    } while (client_fd < 0);

    if (client_fd < 0) {
        printf("\r\nERROR on accept\r\n");
        return -1;
    } else {
        timeout = 3000;
        lwip_setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        timeout = 30000;
        lwip_setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
        lwip_setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
        lwip_setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
        printf("\r\nA client connected to this server :\r\n[PORT]: %d\r\n[IP]:%s\r\n\r\n", ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr.s_addr));
        return client_fd;
    }
}

int get_receive(int sock, uint8_t *data, int length, int flag, uint32_t *peer_addr, uint16_t *peer_port) {
    int ret = 0;
    struct sockaddr from;
    socklen_t fromlen;

    uint8_t backup_recvtimeout = 0;
    int backup_recv_timeout, recv_timeout;
    socklen_t len;

    if (flag & 0x01) {
        // for MSG_PEEK, we try to peek packets by changing receiving timeout to 10ms
        ret = lwip_getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &backup_recv_timeout, &len);
        if (ret >= 0) {
            recv_timeout = 10;
            ret = lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
            if (ret >= 0) {
                backup_recvtimeout = 1;
            }
        }
    }
    ret = lwip_recvfrom(sock, data, length, flag, &from, &fromlen);
    if (ret >= 0) {
        if (peer_addr != NULL) {
            *peer_addr = ((struct sockaddr_in *)&from)->sin_addr.s_addr;
        }
        if (peer_port != NULL) {
            *peer_port = ntohs(((struct sockaddr_in *)&from)->sin_port);
        }
    }

    if ((flag & 0x01) && (backup_recvtimeout == 1)) {
        // restore receiving timeout
        lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &backup_recv_timeout, sizeof(recv_timeout));
    }

    return ret;
}

int get_sock_errno(int sock) {
    int so_error;
    socklen_t len = sizeof(so_error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
    return so_error;
}

int set_sock_recv_timeout(int sock, int timeout) {
    return lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

void stop_socket(int sock) {
    lwip_close(sock);
}

int send_data(int sock, const uint8_t *data, uint16_t len) {
    int ret;
    ret = lwip_write(sock, data, len);
    return ret;
}

int sendto_data(int sock, const uint8_t *data, uint16_t len, uint32_t peer_ip, uint16_t peer_port) {
    int ret;
    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = peer_ip;
    peer_addr.sin_port = htons(peer_port);

    ret = lwip_sendto(sock, data, len, 0, ((struct sockaddr *)&peer_addr), sizeof(struct sockaddr_in));

    return ret;
}

int start_client(uint32_t ipAddress, uint16_t port, uint8_t protMode) {
    int enable = 1;
    int timeout;
    int _sock;

    if (protMode == 0) {  // TCP
        _sock = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    } else {
        _sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    } 
    if (_sock < 0) {
        printf("\r\nERROR opening socket\r\n");
        return -1;
    }
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = ipAddress;
    serv_addr.sin_port = htons(port);
    
    if (protMode == 0) {  //TCP MODE
        if (connect(_sock, ((struct sockaddr *)&serv_addr), sizeof(serv_addr)) == 0) {
            printf("\r\nConnect to Server successfully!\r\n");
            timeout = 3000;
            lwip_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            timeout = 30000;
            lwip_setsockopt(_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            lwip_setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
            lwip_setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
            return _sock;
        } else {
            printf("\r\nConnect to Server failed!\r\n");
            stop_socket(_sock);
            return -1;
        }
    } else {
        //printf("\r\nUdp client setup Server's information successful!\r\n");
    }
    return _sock;
}


int start_clientv6(uint32_t *ipv6Address, uint16_t port, uint8_t protMode) {
    int enable = 1;
    int timeout;
    int _sock;
    struct sockaddr_in6 serv_addr6;

    //create socket    
    if (protMode == 0) {  // TCP
        _sock = lwip_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    } else {
        _sock = lwip_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    } 
    if (_sock < 0) {
        printf("\r\n[ERROR] Create socket failed\n");
        return -1;
    } 
    printf("\n\r[INFO] Create socket successfully\n");

    // initialize structure dest
    memset(&serv_addr6, 0, sizeof(serv_addr6));
    serv_addr6.sin6_family = AF_INET6;  
    serv_addr6.sin6_port = htons(port);
    //inet_pton(AF_INET6, TCP_SERVER_IP_TEST, &(serv_addr6.sin6_addr));
    for (int xxx = 0; xxx < 4; xxx++) {  // IPv6 address
        serv_addr6.sin6_addr.un.u32_addr[xxx] = ipv6Address[xxx];
    }
#if 0 // debug info
    printf("[INFO]ard_socket.c:  serv_addr6.sin6_family:     %x \n\r", serv_addr6.sin6_family);
    printf("[INFO]ard_socket.c:  serv_addr6.sin6_port  :     %x \n\r", serv_addr6.sin6_port);
    printf("ipv6Address0     %x \n\r", ipv6Address[0]);
    printf("ipv6Address 1    %x \n\r", ipv6Address[1]);
    printf("ipv6Address  2   %x \n\r", ipv6Address[2]);
    printf("ipv6Address   3  %x \n\r", ipv6Address[3]);
    printf("yyyyyyyyyyyyyyyyy0     %x \n\r", serv_addr6.sin6_addr.un.u32_addr[0]);
    printf("yyyyyyyyyyyyyyyyy 1    %x \n\r", serv_addr6.sin6_addr.un.u32_addr[1]);
    printf("yyyyyyyyyyyyyyyyy  2   %x \n\r", serv_addr6.sin6_addr.un.u32_addr[2]);
    printf("yyyyyyyyyyyyyyyyy   3  %x \n\r", serv_addr6.sin6_addr.un.u32_addr[3]);
#endif
    // connection starts
    if (protMode == 0) {  //TCP MODE
        if (connect(_sock, (struct sockaddr *)(&serv_addr6), sizeof(serv_addr6)) == -1){
            printf("\n\r[ERROR] Connect to server failed\n");
        }
        printf("[INFO] Connect to server successfully\n");
    
        if (connect(_sock, (struct sockaddr *)(&serv_addr6), sizeof(serv_addr6)) == 0) {
            printf("\r\n[INFO] Connect to Server successfully!\r\n");
            timeout = 3000;
            lwip_setsockopt(_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            timeout = 30000;
            lwip_setsockopt(_sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
            lwip_setsockopt(_sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable));
            lwip_setsockopt(_sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable));
            return _sock;
        } else {
            printf("\r\n[ERROR] Connect to Server failed!\r\n");
            stop_socket(_sock);
            return -1;
        }
    } else {
       //printf("\r\nUdp client setup Server's information successful!\r\n");
    }
    return _sock;
}

int enable_ipv6(void){
    EXAMPLE_IPV6 = 1;        // turn on all ipv6 related functions
    return EXAMPLE_IPV6;
}

int get_ipv6_status(void){
    return EXAMPLE_IPV6;
}
/*
void ipv6_create_socket(int fd, int doamin, int type, int protocol){
    fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (fd == -1){
        printf("\n\r[ERROR] Create socket failed\n");
    }
    printf("\n\r[INFO] Create socket successfully\n");
}*/

void ipv6_tcp_client(void) {

    int client_fd;
    struct sockaddr_in6 ser_addr;
    char recv_data[MAX_RECV_SIZE];
    char send_data[MAX_SEND_SIZE] = "Hi Server!!";
    
    //create socket
    if ((client_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");
   
    //ipv6_create_socket(client_fd);

    //initialize value in dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(TCP_SERVER_PORT);
    inet_pton(AF_INET6, TCP_SERVER_IP, &(ser_addr.sin6_addr));
    

    //Connecting to server
    if (connect(client_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1) {
        printf("\n\r[ERROR] Connect to server failed\n");
    }
    printf("[INFO] Connect to server successfully\n");

    while (1) {
        //Send data to server
        if (send(client_fd, send_data, MAX_SEND_SIZE, 0) == -1) {
            printf("\n\r[ERROR] Send data failed\n");
        } else {
            printf("\n\r[INFO] Send data to server successfully\n");
        }

        //Receive data from server response
        if (recv(client_fd, recv_data, MAX_RECV_SIZE, 0) <= 0) {
            //printf("\n\r[ERROR] Receive data failed\n");
        } else {
            printf("\n\r[INFO] Receive from server: %s\n", recv_data);
        }
        vTaskDelay(1000);
    }
    closesocket(client_fd);
    return;/*
*/
}

void ipv6_tcp_server(void) {
    int server_fd, client_fd;
    struct sockaddr_in6 ser_addr, client_addr;
    //int addrlen = sizeof(struct sockaddr_in6);
    socklen_t addrlen = sizeof(struct sockaddr_in6);
    char send_data[MAX_SEND_SIZE] = "Hi client!!";
    char recv_data[MAX_RECV_SIZE];

    
    //create socket
    if ((server_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");
    
    //ipv6_create_socket(server_fd);

    //initialize structure dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(TCP_SERVER_PORT);
    ser_addr.sin6_addr = (struct in6_addr)IN6ADDR_ANY_INIT;

    //Assign a port number to socket
    if (bind(server_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) != 0) {
        printf("\n\r[ERROR] Bind socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Bind socket successfully\n");

    //Make it listen to socket with max 20 connections
    if (listen(server_fd, 20) != 0) {
        printf("\n\r[ERROR] Listen socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Listen socket successfully\n");

    //Accept
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen)) == -1) {
        printf("\n\r[ERROR] Accept connection failed\n");
        closesocket(server_fd);
        closesocket(client_fd);
        return;
    }
    printf("\n\r[INFO] Accept connection successfully\n");

    while (1) {
        memset(recv_data, 0, MAX_RECV_SIZE);
        if (recv(client_fd, recv_data, MAX_RECV_SIZE, 0) > 0) {
            printf("\n\r[INFO] Receive data : %s\n", recv_data);
            //Send Response
            if (send(client_fd, send_data, MAX_SEND_SIZE, 0) == -1) {
                printf("\n\r[ERROR] Send data failed\n");
            } else {
                printf("\n\r[INFO] Send data successfully\n");
            }
        }
    }
    closesocket(client_fd);
    closesocket(server_fd);
    return;
}

void ipv6_udp_server(void) {
    int server_fd;
    struct sockaddr_in6 ser_addr, client_addr;

    socklen_t addrlen = sizeof(struct sockaddr_in6);

    char send_data[MAX_SEND_SIZE] = "Hi client!";
    char recv_data[MAX_RECV_SIZE];

    //create socket
    if ((server_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");

    //ipv6_create_socket(server_fd);

    
    //initialize structure dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(UDP_SERVER_PORT);
    ser_addr.sin6_addr = (struct in6_addr)IN6ADDR_ANY_INIT;

    //Assign a port number to socket
    if (bind(server_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) != 0) {
        printf("\n\r[ERROR] Bind socket failed\n");
        closesocket(server_fd);
        return;
    }
    printf("\n\r[INFO] Bind socket successfully\n");

    while (1) {
        memset(recv_data, 0, MAX_RECV_SIZE);
        if (recvfrom(server_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&client_addr, &addrlen) > 0) {
            printf("\n\r[INFO] Receive data : %s\n", recv_data);
            //Send Response
            if (sendto(server_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&client_addr, addrlen) == -1) {
                printf("\n\r[ERROR] Send data failed\n");
            } else {
                printf("\n\r[INFO] Send data successfully\n");
            }
        }
    }
    closesocket(server_fd);
    return;
}

void ipv6_udp_client(void) {
    int client_fd;
    struct sockaddr_in6 ser_addr;

    socklen_t addrlen = sizeof(struct sockaddr_in6);

    char recv_data[MAX_RECV_SIZE];
    char send_data[MAX_SEND_SIZE] = "Hi Server!!";

    //create socket
    if ((client_fd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        printf("\n\r[ERROR] Create socket failed\n");
        return;
    }
    printf("\n\r[INFO] Create socket successfully\n");


    //ipv6_create_socket(client_fd);

    //initialize value in dest
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin6_family = AF_INET6;
    ser_addr.sin6_port = htons(UDP_SERVER_PORT);
    inet_pton(AF_INET6, UDP_SERVER_IP, &(ser_addr.sin6_addr));

    while (1) {
        //Send data to server
        if (sendto(client_fd, send_data, MAX_SEND_SIZE, 0, (struct sockaddr *)&ser_addr, sizeof(ser_addr)) == -1) {
            printf("\n\r[ERROR] Send data failed\n");
        } else {
            printf("\n\r[INFO] Send data to server successfully\n");
        }

        //Receive data from server response
        memset(recv_data, 0, MAX_RECV_SIZE);
        if (recvfrom(client_fd, recv_data, MAX_RECV_SIZE, 0, (struct sockaddr *)&ser_addr, &addrlen) <= 0) {
            //printf("\n\r[ERROR] Receive data failed\n");
        } else {
            printf("\n\r[INFO] Receive from server: %s\n", recv_data);
        }
        vTaskDelay(1000);
    }

    closesocket(client_fd);
    return;
}
