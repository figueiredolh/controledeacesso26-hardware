#ifndef WIFI_CONNECTION_H
#define WIFI_CONNECTION_H

#include "WiFi_Credentials.h"
#include <WiFi.h>

void conectarWifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando WiFi..");
  }
  Serial.println("IP: " + (String)WiFi.localIP());
}

#endif