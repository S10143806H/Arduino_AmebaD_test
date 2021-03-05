#include <WiFi.h>

char ssid[] = "xiaomi_test";    // your network SSID (name)
char pass[] = "1234567890";       // your network passwordint keyIndex = 0;         // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;
//IPAddress server(64,233,189,94);  // numeric IP for Google (no DNS)
//IPv6Address server(2001:4860:4860::8888);  // Google Public DNS IPv6 addresses
char server[] = "www.google.com";    // name address for Google (using DNS)

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
    client.TCPServerv6();
    /*
    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:a
    if (client.connect(server, 80)) {
        Serial.println("connected to server");
        // Make a HTTP request:
        client.println("GET /search?q=ameba HTTP/1.1");
        client.println("Host: www.google.com");
        client.println("Connection: close");
        client.println();
    }
    delay(100);*/
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

    // print your WiFi shield's IPv6 address:
    IPv6Address ipv6 = WiFi.localIPv6();
    //Serial.print("IPv6 Address: ");
    //Serial.println(ipv6);
    
    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

