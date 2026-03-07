void setModoExclusaoTrue(){
  if(sensorOcupado){
    Serial.print("Sensor ocupado no momento");
    Serial.println("Modo de exclusão desabilitado");
    //enviar ao mqtt status do sensor ocupado
    enviarDadosExclusaoBiometriaMqtt(1); //Código de Erro 1 - sensor ocupado
    return;
  }
  modoExcluirBiometria = true;
  sensorOcupado = true;
}

void ExcluirBiometria(){/* 
  bool idSensorGravado = idSensorGravadoNaMemoria(idSensorExcluir);

  if(!idSensorGravado){
    Serial.print("Nenhum dado foi gravado no ID informado");
    enviarDadosExclusaoBiometriaMqtt(2); //2 - sensor com nenhum template gravado - slot vazio
    setModoExclusaoFalse();
    return;
  } */

  bool sucesso = excluirModeloIdBiometria(idSensorExcluir);

  if(!sucesso){
    // Tenta uma segunda vez se a primeira falhar (Retry Logic)
    delay(50);
    sucesso = excluirModeloIdBiometria(idSensorExcluir);
  }

  if(sucesso){
    enviarDadosExclusaoBiometriaMqtt(idSensorExcluir, 0); 
  }
  else{
    enviarDadosExclusaoBiometriaMqtt(idSensorExcluir, 2); //2 - erro ao excluir modelo/template da memória flash do sensor
  }

  setModoExclusaoFalse();  
  return;
}

void setModoExclusaoFalse(){
  modoExcluirBiometria = false;
  sensorOcupado = false;
}

/* bool idSensorGravadoNaMemoria(int idSensor){
  uint8_t status;

  delay(2);
  status = finger.loadModel(idSensor);

  if (status == FINGERPRINT_OK) {
    Serial.println("Posição já gravada!");
    return true;
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erro de comunicação");
  } 
  else {
    Serial.print("Posição "); Serial.print(idSensor); Serial.print(" VAZIA");
  }

  return false; //retorna nenhum id vazio
} */

bool excluirModeloIdBiometria(int idSensor){
  uint8_t status = -1;

  status = finger.deleteModel(idSensor);

  if (status == FINGERPRINT_OK) {
    Serial.println("Excluído");
    return true;
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erro de comunicação");
  } 
  else if (status == FINGERPRINT_BADLOCATION) {
    Serial.println("Falha em excluir nessa localização");
  } 
  else if (status == FINGERPRINT_FLASHERR) {
    Serial.println("Erro ao escrever na memória flash");
  } 
  else {
    Serial.print("Erro desconhecido: 0x");
    Serial.println(status, HEX);
  }

  return false;
}