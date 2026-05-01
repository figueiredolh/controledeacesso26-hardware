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

int pinTouch = 19;
volatile bool dedoNoSensor = false;

int gpioStatusPorta = 26;
volatile bool portaAberta = false;

int pinReleAbrirPorta = 23;

bool modoCadastroBiometria = false;
volatile bool processarSalvarTemplate = false;

bool modoExcluirBiometria = false;
volatile int idSensorExcluir;

bool sensorOcupado = false;

MQTTClient clientMQTT(1280);
WiFiClientSecure wifiClient;

void connectMqtt() {
  // Loop until we're reconnected
  while (!clientMQTT.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clientMQTT.connect("ESP32Client", mqtt_server_username, mqtt_server_password)) {
      Serial.println("connected");
      // Subscribe
      clientMQTT.subscribe("controledeacesso26/biometria/cadastro/sensor1");
      clientMQTT.subscribe("controledeacesso26/biometria/excluir/sensor1");
      clientMQTT.subscribe("controledeacesso26/biometria/porta/abrir");
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
  Serial.println("Mensagem no tópico: ");
  Serial.print(topic);
  Serial.print(". Mensagem: ");
  Serial.print(payload);

  JsonDocument payloadJson;
  deserializeJson(payloadJson, payload);

  //solicitação de cadastro de biometria
  if (String(topic) == "controledeacesso26/biometria/cadastro/sensor1") {
    if (payloadJson["ColetarDados"] == true) {  //payload recebido informa que os dados podem ser coletados e posteriormente enviados
      if (payloadJson["CadastroEtapa"] == 0) {
        Serial.println("Modo de Cadastro Habilitado");
        setModoCadastroTrue();  //ativa o modo de cadastro no código principal
      }
      if (payloadJson["CadastroEtapa"] == 1) {
        processarSalvarTemplate = true;
      }
    }
    if (payloadJson["ColetarDados"] == false) {
      Serial.println("Modo de Cadastro Desabilitado");
      setModoCadastroFalse();  //desativa o modo de cadastro no código principal
    }
  }

  if (String(topic) == "controledeacesso26/biometria/excluir/sensor1") {
    //int idSensorPayload = payloadJson["IdSensor"];
    if (payloadJson["IdSensor"] > 0) {
      Serial.println("Modo de Exclusão Habilitada");
      idSensorExcluir = payloadJson["IdSensor"];
      setModoExclusaoTrue();
    } else {
      Serial.println("Modo de Exclusão Desabilitado");
      setModoExclusaoFalse();
    }
  }

  if (String(topic) == "controledeacesso26/biometria/porta/abrir") {
    if (!portaAberta) {
      digitalWrite(pinReleAbrirPorta, HIGH);
      delay(2000);
      digitalWrite(pinReleAbrirPorta, LOW);
      delay(2000);
    } else {
      Serial.println("Porta fechada");
    }
  }
}

void limparBufferSensor() {
  while (mySerial.available()) {
    mySerial.read();
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

  pinMode(gpioStatusPorta, INPUT_PULLUP);
  pinMode(pinTouch, INPUT);
  pinMode(pinReleAbrirPorta, OUTPUT);
  digitalWrite(pinReleAbrirPorta, LOW);
  finger.LEDcontrol(false);
}

void loop() {
  clientMQTT.loop();

  if (!clientMQTT.connected()) {
    connectMqtt();
  }

  if (digitalRead(gpioStatusPorta) == HIGH) {
    portaAberta = true;
  }
  if (digitalRead(gpioStatusPorta) == LOW) {
    portaAberta = false;
  }

  if (digitalRead(pinTouch) == HIGH) {
    dedoNoSensor = true;
  }
  if (digitalRead(pinTouch) == LOW) {
    dedoNoSensor = false;
  }

  if (modoCadastroBiometria) {
    limparBufferSensor();
    //chamar função de cadastro de biometria adafruit
    CadastrarBiometria();
  }
  if (modoExcluirBiometria) {
    //chamar função de excluir biometria adafruit
    limparBufferSensor();
    ExcluirBiometria();
  } else {
    //chamar função de leitura de biometria adafruit
    limparBufferSensor();
    verificarBiometria();
  }
}