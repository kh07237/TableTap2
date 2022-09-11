//参考サイト
// https://garretlab.web.fc2.com/arduino/esp32/reference/libraries/WiFi/WiFiSTAClass_config.html
#include <M5Atom.h>
#include <WiFi.h>

#include "secretinfo.h"
/*
secretinfo.hには下記の情報を記載する
const char* ssid = "xxxxxxxxxx";
const char* password = "xxxxxxxxxx";
*/

const IPAddress local_IP(192, 168, 10, 91);
const IPAddress gateway(192, 168, 10, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress primaryDNS(8, 8, 8, 8); //optional
const IPAddress secondaryDNS(8, 8, 4, 4); //optional

void wifi_setup(void) 
{
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  } 
  // WiFi接続
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi Connected!");
  Serial.print("WiFi Connect To: ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  

}
