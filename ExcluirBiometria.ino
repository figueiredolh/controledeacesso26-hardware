int idSensorExcluir;

void ExcluirBiometria(){
  bool idSensorGravado = idSensorGravadoNaMemoria(idSensorExcluir);

  if(!idSensorGravado){
    Serial.print("Nenhum dado foi gravado no ID informado");
    setModoExclusaoFalse();
    enviarDadosExclusaoBiometriaMqtt(false, 1); //1 - sensor com nenhum template gravado - slot vazio
    return;
  }

  bool sucesso = excluirModeloIdBiometria(idSensorExcluir);

  if(!sucesso){
    setModoExclusaoFalse();
    enviarDadosExclusaoBiometriaMqtt(false, 2); //2 - erro ao excluir modelo/template da memória flash do sensor
    return;
  }

  enviarDadosExclusaoBiometriaMqtt(true);
  setModoExclusaoFalse();
  
  return;
}

void setModoExclusaoTrue(int idSensor){
  if(sensorOcupado){
    Serial.print("Sensor ocupado no momento");
    Serial.println("Modo de exclusão desabilitado");
    //enviar ao mqtt status do sensor ocupado
    enviarDadosExclusaoBiometriaMqtt(false, 0); //0 - sensor ocupado
    return;
  }
  modoExcluirBiometria = true;
  sensorOcupado = true;
  idSensorExcluir = idSensor;
}

void setModoExclusaoFalse(){
  modoExcluirBiometria = false;
  sensorOcupado = false;
}

bool idSensorGravadoNaMemoria(int idSensor){
  uint8_t status;

  delay(10);
  status = finger.loadModel(idSensor);

  if (status == FINGERPRINT_OK) {
    Serial.println("Posição já gravada!");
    return true;
  } 
  else if (status == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erro de comunicação");
    return false;
  } 
  else {
    Serial.print("Posição "); Serial.print(idSensor); Serial.print(" VAZIA");
    return false;
  }

  return false; //retorna nenhum id vazio
}

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