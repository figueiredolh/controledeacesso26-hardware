void enviarDadosCadastroBiometriaMqtt(int idSensor, String templateBiometriaHex){
  JsonDocument doc; 
  
  doc["UsuarioTemplate"] = templateBiometriaHex;
  doc["IdSensor"] = idSensor;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/controle/enviar", payloadJsonString.c_str(), false, 1);
}