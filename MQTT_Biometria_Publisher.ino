void enviarDadosCadastroBiometriaMqtt(int idSensor, String templateBiometriaHex){
  JsonDocument doc; 
  
  doc["UsuarioTemplate"] = templateBiometriaHex;
  doc["IdSensor"] = idSensor;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/controle/enviar", payloadJsonString.c_str(), false, 1);
}

void enviarDadosExclusaoBiometriaMqtt(bool excluido){
  JsonDocument doc; 

  doc["Excluido"] = excluido;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/excluir/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}

//sobrecarga
void enviarDadosExclusaoBiometriaMqtt(bool excluido, uint8_t codigoErro){
  JsonDocument doc; 

  doc["Excluido"] = excluido;
  doc["Codigo"] = codigoErro;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/excluir/sensor1/enviar", payloadJsonString.c_str(), false, 1);
}