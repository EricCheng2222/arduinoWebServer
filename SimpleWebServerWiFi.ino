/*
  WiFi Web Server LED Blink

 A simple web server that lets you blink an LED via the web.
 This sketch will print the IP address of your WiFi module (once connected)
 to the Serial Monitor. From there, you can open that address in a web browser
 to turn on and off the LED on pin 9.

 If the IP address of your board is yourAddress:
 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

 This example is written for a network using WPA encryption. For
 WEP or WPA, change the WiFi.begin() call accordingly.

 Circuit:
 * Board with NINA module (Arduino MKR WiFi 1010, MKR VIDOR 4000 and UNO WiFi Rev.2)
 * LED attached to pin 9

 created 25 Nov 2012
 by Tom Igoe
 */
#include <SPI.h>
#include <WiFiNINA.h>
#include <IRremote.h>

#include "arduino_secrets.h" 
#include "PinDefinitionsAndMore.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int isOn = 1;
int currentTemp = 25;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  Serial.begin(115200);      // initialize serial communication
  
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)  || defined(ARDUINO_attiny3217)
  delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
  IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
  Serial.print(F("Ready to send IR signals at pin "));
#if defined(ARDUINO_ARCH_STM32) || defined(ESP8266)
  Serial.println(IR_SEND_PIN_STRING);
#else
  Serial.print(IR_SEND_PIN);
#endif

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status
}


void loop() {
  WiFiClient client = server.available();   // listen for incoming clients
  
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/H\">here</a> turn the LED on pin 9 on<br>");
            client.print("Click <a href=\"/L\">here</a> turn the LED on pin 9 off<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /H") && isOn==0) {
          currentTemp = 25;
          isOn = 1;
          sendTurnOnSignal();               // GET /H turns the AC on
        }
        else if (currentLine.endsWith("GET /L") && isOn==1) {
          isOn = 0;
          sendTurnOnSignal();                // GET /L turns the AC off
        }
        else if (currentLine.endsWith("GET /status")){
          client.print(String("HTTP/1.1 200 OK")+"\n\n" + isOn + "\r");
        }
        else if (currentLine.endsWith("GET /tempstatus")){
          client.print(String("HTTP/1.1 200 OK")+"\n\n" + currentTemp + "\r");
        }
        else{
          String result;
          for(int i = 20; i<30; i++) {
            result = "GET /settemp/";
            result.concat(String(i));
            if(currentLine.endsWith(result)){
              while(true){
                if(currentTemp<i){
                  Serial.println("temp goes up");
                  sendTempUpSignal();
                  currentTemp++;
                }
                if(currentTemp>i){
                  Serial.println("temp goes down");
                  sendTempDownSignal();
                  currentTemp--;
                }
                if(currentTemp == i){
                  break;
                }
              }
            }
          }
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void sendTurnOnSignal(){
    static const uint8_t NEC_KHZ = 38; // 38kHz carrier frequency for the NEC protocol
    static const uint16_t irSignal[] = {3224, 1632, 444, 416, 436, 428, 436, 1268, 436, 1244, 464, 424, 436, 424, 440, 424, 440, 420, 444, 1264, 440, 1240, 468, 1264, 440, 1264, 444, 1264, 440, 1264, 444, 1264, 440, 1268, 436, 424, 440, 420, 444, 1264, 444, 1236, 468, 1264, 444, 1264, 440, 1264, 440, 1268, 440, 424, 440, 420, 440, 1264, 444, 1236, 468, 1264, 444, 1264, 440, 1240, 468, 1240, 464, 424, 440, 420, 444, 416, 436, 1244, 464, 1244, 460, 1272, 444, 1264, 440, 1236, 472, 416, 436, 428, 436, 1268, 440, 1240, 464, 1268, 436, 424, 440, 424, 440, 1268, 440, 420, 440, 420, 444, 1264, 444, 1236, 468, 1264, 444, 1260, 444, 1264, 440, 1240, 468, 420, 444, 416, 436, 1272, 432, 428, 436, 424, 440, 424, 440, 420, 444, 420, 440, 1264, 444, 420, 444, 416, 436, 424, 440, 424, 440, 420, 440, 424, 440, 1268, 440}; // Using exact NEC timing
    IrSender.sendRaw(irSignal, sizeof(irSignal) / sizeof(irSignal[0]), NEC_KHZ); // Note the approach used to automatically calculate the size of the array.
    
}

void sendTempUpSignal(){
    static const uint8_t NEC_KHZ = 38;
    static const uint16_t irSignal[] = {3220, 1636, 440, 420, 444, 420, 432, 1272, 444, 1264, 444, 392, 460, 428, 436, 424, 440, 420, 440, 1268, 444, 416, 444, 1264, 440, 1268, 436, 1244, 464, 1244, 460, 1244, 464, 1244, 472, 416, 436, 428, 436, 1240, 464, 1244, 464, 1244, 460, 1244, 460, 1248, 468, 1236, 472, 416, 436, 428, 436, 1268, 436, 1244, 464, 1244, 460, 1244, 464, 1244, 472, 1236, 468, 420, 444, 416, 436, 424, 440, 1268, 436, 1244, 464, 1240, 464, 1244, 464, 1244, 460, 428, 436, 424, 436, 1244, 464, 1240, 464, 1244, 464, 1240, 464, 428, 436, 424, 440, 420, 444, 420, 440, 1240, 468, 420, 444, 1232, 472, 1236, 468, 1240, 468, 1240, 464, 420, 444, 420, 444, 1236, 468, 420, 432, 428, 436, 428, 436, 424, 440, 424, 440, 1240, 464, 424, 440, 420, 444, 416, 436, 428, 436, 424, 436, 428, 436, 1244, 464};
    IrSender.sendRaw(irSignal, sizeof(irSignal)/sizeof(irSignal[0]), NEC_KHZ);
}

void sendTempDownSignal(){
    static const uint16_t irSignal[] = {3224, 1632, 436, 424, 440, 424, 440, 1240, 464, 1240, 468, 420, 444, 416, 444, 420, 436, 400, 460, 1272, 436, 424, 440, 1240, 464, 1248, 468, 1236, 472, 1236, 472, 1232, 472, 1236, 472, 416, 436, 424, 440, 1240, 464, 1244, 464, 1240, 464, 1244, 464, 1244, 460, 1244, 464, 424, 440, 424, 440, 1236, 468, 1240, 468, 1236, 468, 1240, 468, 1240, 464, 1244, 464, 420, 444, 420, 444, 416, 436, 1272, 432, 1272, 436, 1244, 460, 1248, 460, 1244, 472, 416, 436, 428, 436, 1240, 468, 1264, 440, 424, 440, 1240, 468, 1236, 468, 1268, 440, 420, 444, 420, 440, 1236, 472, 416, 436, 1244, 464, 1244, 460, 1248, 460, 1244, 472, 416, 436, 428, 436, 1240, 468, 420, 440, 424, 440, 420, 444, 420, 444, 416, 436, 1272, 436, 424, 440, 424, 436, 424, 440, 1264, 444, 420, 444, 1236, 468, 420, 444};
    static const uint8_t NEC_KHZ = 38;
    IrSender.sendRaw(irSignal, sizeof(irSignal)/sizeof(irSignal[0]), NEC_KHZ);
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}
