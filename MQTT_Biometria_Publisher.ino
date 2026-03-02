//void enviarDadosCadastroBiometriaMqtt(uint8_t *templateDadosBiometria, int idSensor){
void enviarDadosCadastroBiometriaMqtt(int idSensor){
  JsonDocument doc; 
  
  //doc["UsuarioTemplate"] = templateDadosBiometria;
  doc["IdSensor"] = idSensor;

  String payloadJsonString;
  serializeJson(doc, payloadJsonString);

  clientMQTT.publish("controledeacesso26/biometria/cadastro/controle/enviar", payloadJsonString.c_str(), false, 1);
}