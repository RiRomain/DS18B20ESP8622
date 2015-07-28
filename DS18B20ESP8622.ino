#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
 
const char* ssid = "BabyAieNiMua";
const char* password = "ABCDEDCBA";
MDNSResponder mdns;

ESP8266WebServer server(80);
WiFiClient client;

const char* thingSpeakAddress2 = "api.thingspeak.com";
String writeAPIKey = "NLFS6YYV742V31FF";
const int updateThingSpeakInterval = 30 * 1000;      // Time interval in milliseconds to update ThingSpeak (number of seconds * 1000 = interval)

// Variable Setup
long lastConnectionTime = 0; 
//boolean lastConnected = false;
//int failedCounter = 0;
String analogValue0 ="";
float celsius;

OneWire  ds(2);  // on pin 2 (a 4.7K resistor is necessary)

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266! Actual temperature "+String(celsius, DEC)+" last connection "+lastConnectionTime+" times");
}
 
void setup(void){
  
  
 // Serial.begin(115200);
  WiFi.begin(ssid, password);
//  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
 //   Serial.print(".");
  }
//  Serial.println("");
//  Serial.print("Connected to ");
//  Serial.println(ssid);
//  Serial.print("IP address: ");
//  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
//    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });
  
  server.begin();
//  Serial.println("HTTP server started");
}
 
void loop(void){
  server.handleClient();
    byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float fahrenheit, celsiuss;

    if ( !ds.search(addr)) {
 //   Serial.println("No more addresses.");
//    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
 //     Serial.println("CRC is not valid!");
      return;
  }
//  Serial.println();

  ds.reset();
  ds.select(addr);
  ds.write(0x44);        // start conversion, use ds.write(0x44,1) with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  
  celsiuss = round(celsius);
  analogValue0 = String(celsiuss, DEC);
  
  // Update ThingSpeak
  if(millis() - lastConnectionTime > updateThingSpeakInterval)
  {
    updateThingSpeak("field1="+analogValue0);
  }
} 


void updateThingSpeak(String tsData)
{
 
  if (client.connect(thingSpeakAddress2,80)) {  //   "184.106.153.149" or api.thingspeak.com
    /*String postStr = writeAPIKey;
           postStr +="&field1=";
           postStr += String(t);
           postStr +="&field2=";
           postStr += String(h);
           postStr += "\r\n\r\n";*/
 
     client.print("POST /update HTTP/1.1\n"); 
     client.print("Host: api.thingspeak.com\n"); 
     client.print("Connection: close\n"); 
     client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n"); 
     client.print("Content-Type: application/x-www-form-urlencoded\n"); 
     client.print("Content-Length: "); 
     client.print(tsData.length()); 
     client.print("\n\n"); 
     client.print(tsData);
  }
  client.stop();
  lastConnectionTime = millis(); 
}
