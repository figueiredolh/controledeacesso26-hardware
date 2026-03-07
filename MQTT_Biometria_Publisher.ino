// Cadastrar Biometria
void enviarDadosCadastroBiometriaMqtt(uint8_t idSensor, uint8_t codigoErro){
  JsonDocument doc; 

  doc["IdSensor"] = idSensor;
  doc["CodigoErro"] = codigoErro;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}

void enviarDadosCadastroBiometriaMqtt(uint8_t codigoErro){
  JsonDocument doc; 
  
  doc["IdSensor"] = 0;
  doc["CodigoErro"] = codigoErro;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}

void enviarDadosCadastroBiometriaMqtt(uint8_t idSensor, String templateBiometriaHex){
  JsonDocument doc; 
  
  doc["UsuarioTemplate"] = templateBiometriaHex;
  doc["IdSensor"] = idSensor;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}

// Excluir Biometria

void enviarDadosExclusaoBiometriaMqtt(uint8_t idSensor, uint8_t codigoErro){ //sobrecarga
  JsonDocument doc; 

  doc["IdSensor"] = idSensor;
  doc["CodigoErro"] = codigoErro;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/excluir/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}

void enviarDadosExclusaoBiometriaMqtt(uint8_t codigoErro){ //sobrecarga
  JsonDocument doc;   
  
  doc["IdSensor"] = 0;
  doc["CodigoErro"] = codigoErro;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/excluir/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}