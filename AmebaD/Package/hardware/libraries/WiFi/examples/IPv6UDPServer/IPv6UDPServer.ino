#include <WiFi.h>
#define TCP   1
#define UDP   0

//char ssid[] = "SINGTEL-D45F";    // your network SSID (name)
//char pass[] = "mooxuteeth";       // your network passwordint keyIndex = 0;  
char ssid[] = "xiaomi_test";    // your network SSID (name)
char pass[] = "1234567890";       // your network passwordint keyIndex = 0;  // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
int server_fd;

WiFiClient client;
void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    while (!Serial) {
        ;
    }
    // enable ipv6 macro
    WiFi.enableIPv6();
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

    //client.createSocketV6(server_fd, UDP);
    
    client.UDPServerv6();

}

void loop() {
    // if there are incoming bytes available
    // from the server, read them and print them:
    while (client.available()) {
        char c = client.read();
        Serial.write(c);
    }

    // if the server's disconnected, stop the client:
    if (!client.connected()) {
        Serial.println();
        Serial.println("disconnecting from server.");
        client.stop();

        // do nothing forevermore:
        while (true);
    }
    
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
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

