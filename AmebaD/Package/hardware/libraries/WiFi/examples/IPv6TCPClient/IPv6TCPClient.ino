#include <WiFi.h>
#include "ard_socket.h"

#define TCP   1
#define UDP   0
#define TCP_SERVER_PORT 5003

#if 0
char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
//char ssid[] = "xiaomi_test";    // your network SSID (name)
//char pass[] = "1234567890";       // your network passwordint keyIndex = 0;         // your network key Index number (needed only for WEP)
#endif
char ssid[] = "SINGTEL-D45F";    // your network SSID (name)
char pass[] = "mooxuteeth";       // your network passwordint keyIndex = 0;         // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
//IPAddress server(64,233,189,94);  // numeric IP for Google (no DNS)
//IPv6Address server(2001:4860:4860::8888);  // Google Public DNS IPv6 addresses
char server[] = "www.google.com";    // name address for Google (using DNS)

int client_fd;   
//struct sockaddr_in6 ser_addr;
//char recv_data[MAX_RECV_SIZE];
//char send_data[MAX_SEND_SIZE] = "Hi Server!!";

WiFiClient client;


void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    // check for the presence of the shield:
    if (WiFi.status() == WL_NO_SHIELD) {
        Serial.println("WiFi shield not present");
        // don't continue:
        while (true);
    }

    // attempt to connect to Wifi network:
    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(ssid);
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
        status = WiFi.begin(ssid, pass);

        // wait 10 seconds for connection:
        delay(10000);
    }
    Serial.println("Connected to wifi");
    printWifiStatus();

    // Enable ipv6 function     
    if(client.enableIPv6()){
        if((client.getIPv6Status()) == 1){
            Serial.println("\nEnable IPv6 functions");
        }
        else{
            Serial.println("\nFailed to enable IPv6 functions");
        }
    }

    // Connecting to server
    Serial.println("\nStarting connection to server...");
    /*
    // if you get a connection, report back via serial:
    if (client.connect(server, 80)) {
        Serial.println("connected to server");
        // Make a HTTP request:
        client.println("GET /search?q=ameba HTTP/1.1");
        client.println("Host: www.google.com");
        client.println("Connection: close");
        client.println();
    }
    delay(100);
    */
    
    //client.TCPClientv6();
    //client_fd = start_client_v6(TCP_SERVER_PORT, 0);
    client_fd = client.connect(TCP_SERVER_PORT);
    Serial.println("\nBack to INO...");
    
    client.TCPClientv6();

}

void loop() {
    
}


void printWifiStatus() {
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print link local IPv6 address:
    WiFi.printLocalIPv6();
    
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.println("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

