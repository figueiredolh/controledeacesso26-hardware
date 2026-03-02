#include "WiFi_Connection.h"
#include <WiFiClientSecure.h>
#include "BrokerMQTT_Credentials.h"
#include <MQTT.h>
#include <ArduinoJson.h>
#include <Adafruit_Fingerprint.h>

#define mySerial Serial2

// Pinos RX e TX
#define RXD2 16
#define TXD2 17

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int ledVerde = 4;
bool modoCadastroBiometria = false;

MQTTClient clientMQTT(1024);
WiFiClientSecure wifiClient;

void connectMqtt() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clientMQTT.connect("ESP32Client", mqtt_server_username, mqtt_server_password)) {
      Serial.println("connected");
      // Subscribe
      clientMQTT.subscribe("controledeacesso26/biometria/cadastro/controle");
    } else {
      Serial.print("failed, rc=");
      Serial.print(clientMQTT.returnCode());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callbackMessageReceived(String &topic, String &payload) {
  Serial.println("Messagem no tópico: ");
  Serial.print(topic);
  Serial.print(". Mensagem: ");
  Serial.print(payload);

  JsonDocument payloadJson;
  deserializeJson(payloadJson, payload);

  //solicitação de cadastro de biometria
  if (String(topic) == "controledeacesso26/biometria/cadastro/controle") {      
    if (payloadJson["ColetarDados"] == "true"){ //payload recebido informa que os dados podem ser coletados e posteriormente enviados
      Serial.println("Modo de Cadastro Habilitado");      
      modoCadastroBiometria = true; //ativa o modo de cadastro no código principal
    }
    if (payloadJson["ColetarDados"] == "false"){
      Serial.println("Modo de Cadastro Desabilitado");
      modoCadastroBiometria = false; //desativa o modo de cadastro no código principal
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  mySerial.begin(57600, SERIAL_8N1, RXD2, TXD2);

  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Sensor biométrico encontrado");
  } else {
    Serial.println("Sensor biométrico não encontrado");
    while (1) { delay(1); }
  }

  conectarWifi();
  wifiClient.setInsecure();
  clientMQTT.begin(mqtt_server, mqtt_server_port, wifiClient); 
  clientMQTT.onMessage(callbackMessageReceived);
  
  pinMode(ledVerde, OUTPUT);
}

void loop() {
  clientMQTT.loop();

  if (!clientMQTT.connected()) {
    connectMqtt();
  }

  if(modoCadastroBiometria){
    limparBufferSensor();
    //chamar função de cadastro de biometria adafruit
    CadastrarBiometria();
  }
  else{
    //chamar função de leitura de biometria adafruit
    limparBufferSensor();
  }
}