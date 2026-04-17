void verificarBiometria(){
  if(!dedoNoSensor){
    return;
  }
  
  if(portaAberta){
    finger.LEDcontrol(false);
    return;
  }

  verificarEstadoSensor();
  bool sucessoLerDigital = lerDigital();

  if(sucessoLerDigital){
    sensorOcupado = true;
    bool sucessoBuscarDigital = buscarDigital();

    if(sucessoBuscarDigital){
      uint16_t idDigital = lerIdDigital();
      Serial.println(idDigital);
      //enviar idDigital via mqtt
      verificarBiometriaMqtt(idDigital);
    }

    Serial.println("Retire o dedo do leitor");
    uint8_t statusFingerprint = 0;
    delay(2000);

    while(statusFingerprint != FINGERPRINT_NOFINGER){
      statusFingerprint = finger.getImage();
    }

    finalizarVerificacao(); 
  }
}

void verificarEstadoSensor(){  
  if(sensorOcupado){
    Serial.print("Sensor ocupado no momento");
    Serial.println("Modo de verificação suspenso");
    //enviar ao mqtt status do sensor ocupado
    //enviarDadosExclusaoBiometriaMqtt(1); //Código de Erro 1 - sensor ocupado
    return;
  }
}

void finalizarVerificacao(){
  sensorOcupado = false;
  finger.LEDcontrol(false);
  delay(1000);
}

bool lerDigital(){
  uint8_t statusFingerprint = finger.getImage();
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.println("Imagem capturada");
      delay(10);
      //finger.LEDcontrol(true);
      //enviarDadosCadastroBiometriaMqttFeedback(2);      
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("Detectado nenhum dedo no sensor");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erro de comunicação");
      return false;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Erro ao renderizar imagem");
      return false;
    default:
      Serial.println("Erro desconhecido");
      return false;
  }

  statusFingerprint = finger.image2Tz();
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.println("Imagem convertida");
      delay(10);
      //enviarDadosCadastroBiometriaMqttFeedback(3);
      return true;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erro de comunicação");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Erro desconhecido");
      return false;
  }
}

bool buscarDigital(){
  // OK converted!
  uint8_t statusFingerprint = finger.fingerSearch();
  if (statusFingerprint == FINGERPRINT_OK) {
    Serial.println("Biometria encontrada");
    return true;
  } 
  else if (statusFingerprint == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Erro de comunicação");
    return false;
  } 
  else if (statusFingerprint == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return false;
  } 
  else {
    Serial.println("Unknown error");
    return false;
  }
}

uint16_t lerIdDigital(){
  uint16_t idDigital = finger.fingerID;
  uint16_t idDigitalConfidence = finger.confidence;

  Serial.print("ID encontrado: #");
  Serial.print(idDigital);
  Serial.print(" with confidence of ");
  Serial.println(idDigitalConfidence);

  return idDigital;
}