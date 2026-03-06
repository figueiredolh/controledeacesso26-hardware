void CadastrarBiometria(){
  int idSensor = buscarIdVazio();

  if(idSensor == -1){
    Serial.print("Todos os slots da memória do sensor estão ocupados");
    //publicar mensagem mqtt
    setModoCadastroFalse();
    enviarDadosCadastroBiometriaMqtt(0, "1");
    return;
  };

  uint8_t tentativasLerDigitaisMaximas = 3;
  uint8_t tentativasLerDigitais = 0;
  bool sucesso = false;   

  while(!sucesso && tentativasLerDigitais < tentativasLerDigitaisMaximas && modoCadastroBiometria){
    sucesso = lerDigitaisESalvar(idSensor);

    if(!sucesso && !modoCadastroBiometria){
      Serial.println("Operação cancelada");
      //enviar mensagem mqtt aqui
      setModoCadastroFalse();
      return;
    }

    if(!sucesso){
      tentativasLerDigitais++;
      Serial.print("Falha na tentativa "); 
      Serial.print(tentativasLerDigitais);
      Serial.print("/"); Serial.print(tentativasLerDigitaisMaximas);
      //delay(500);
    } 
  }

  if(!sucesso){
    Serial.println("Limite de tentativas atingido!");
    setModoCadastroFalse();
    // Envia erro para o .NET - linha em baixo
    enviarDadosCadastroBiometriaMqtt(0, "2"); //código para limite de tentativa - temporário
    return;
  }

  String templateBiometriaHex = ExtrairTemplateBiometria(idSensor);

  enviarDadosCadastroBiometriaMqtt(idSensor, templateBiometriaHex);
  setModoCadastroFalse();
  
  return;
}

int buscarIdVazio(){
  uint8_t p; //cada status é representado por um valor inteiro
  int ultimaPosicaoDeMemoria = 300;

  for(int id = 1; id <= ultimaPosicaoDeMemoria; id++){
    delay(10);
    p = finger.loadModel(id); //lê o status de cada id, iterando-no

    if (p == FINGERPRINT_OK) {
      Serial.println("Posição ocupada!");
      continue;
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Erro de comunicação");
    } else {
      // Se o status for diferente de OK, geralmente a posição está vazia/inválida
      Serial.print("Posição "); Serial.print(id); Serial.print(" VAZIA");
      return id;
    }
  }

  return -1; //retorna nenhum id vazio
}

bool lerDigitaisESalvar(uint16_t id){
  int statusFingerprint = -1;
  
  Serial.print("Aguardando leitura de biometria do usuário no ID: "); Serial.println(id);

  while (statusFingerprint != FINGERPRINT_OK) {
    clientMQTT.loop();

    if(!modoCadastroBiometria){
      //limparBufferSensor();
      return false;
    }

    statusFingerprint = finger.getImage();
    switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.println("Imagem capturada");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
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
  }

  // OK success!

  statusFingerprint = finger.image2Tz(1);
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.println("Imagem convertida");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
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

  Serial.println("Retire o dedo do leitor");
  delay(2000);
  statusFingerprint = 0;
  while (statusFingerprint != FINGERPRINT_NOFINGER) {
    clientMQTT.loop();

    if(!modoCadastroBiometria){
      //limparBufferSensor();
      return false;
    }

    statusFingerprint = finger.getImage();
  }
  Serial.print("ID: "); Serial.println(id);
  statusFingerprint = -1;
  Serial.println("Coloque o dedo novamente no leitor");
  while (statusFingerprint != FINGERPRINT_OK) {
    clientMQTT.loop();

    if(!modoCadastroBiometria){
      //limparBufferSensor();
      return false;
    }

    statusFingerprint = finger.getImage();
    switch (statusFingerprint) {
      case FINGERPRINT_OK:
        Serial.println("Imagem capturada");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return false;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        return false;
      default:
        Serial.println("Unknown error");
        return false;
    }
  }

  // OK success!

  statusFingerprint = finger.image2Tz(2);
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.println("Imagem convertida");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return false;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return false;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return false;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return false;
    default:
      Serial.println("Unknown error");
      return false;
  }

  // OK converted!
  Serial.print("Criando modelo para o ID: ");  Serial.println(id);

  statusFingerprint = finger.createModel();
  if (statusFingerprint == FINGERPRINT_OK) {
    Serial.println("Digitais coincidem");
  } else if (statusFingerprint == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (statusFingerprint == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Digitais não coincidem");
    return false;
  } else {
    Serial.println("Erro desconhecido");
    return false;
  }

  Serial.print("ID "); Serial.println(id);
  statusFingerprint = finger.storeModel(id);
  if (statusFingerprint == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (statusFingerprint == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return false;
  } else if (statusFingerprint == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return false;
  } else if (statusFingerprint == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return false;
  } else {
    Serial.println("Unknown error");
    return false;
  }

  /* Serial.print("Attempting to get #"); Serial.println(id);
  statusFingerprint = finger.getModel();
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.print("Transferindo template do ID "); Serial.print(id); Serial.print("para o Serial");
      break;
    default:
      Serial.print("Erro desconhecido "); Serial.println(statusFingerprint);
      return false;
  } */

  //Serial.println("------------------------------------");
  Serial.print("Carregando dados do ID #"); Serial.println(id);
  statusFingerprint = finger.loadModel(id);
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.print("Template "); Serial.print(id); Serial.println(" loaded");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Erro de comunicação");
      return false;
    default:
      Serial.print("Erro desconhecido "); Serial.println(statusFingerprint);
      return false;
  }

  Serial.print("Attempting to get #"); Serial.println(id);
  statusFingerprint = finger.getModel();
  switch (statusFingerprint) {
    case FINGERPRINT_OK:
      Serial.print("Transferindo template do ID "); Serial.print(id); Serial.print("para o Serial");
      break;
    default:
      Serial.print("Erro desconhecido "); Serial.println(statusFingerprint);
      return false;
  }

  return true;
}

String ExtrairTemplateBiometria(int id){
  // one data packet is 267 bytes. in one data packet, 11 bytes are 'usesless' :D
  uint8_t bytesReceived[534]; // 2 data packets
  memset(bytesReceived, 0xff, 534);

  uint32_t starttime = millis();
  int i = 0;
  while (i < 534 && (millis() - starttime) < 20000) {
    if (mySerial.available()) {
      bytesReceived[i++] = mySerial.read();
    }
  }
  Serial.print(i); Serial.println(" bytes read.");
  Serial.println("Decoding packet...");

  uint8_t templateBiometria[512];
  memset(templateBiometria, 0xff, 512);

  // filtering only the data packets
  int uindx = 9, index = 0;
  memcpy(templateBiometria + index, bytesReceived + uindx, 256);   // first 256 bytes
  uindx += 256;       // skip data
  uindx += 2;         // skip checksum
  uindx += 9;         // skip next header
  index += 256;       // advance pointer
  memcpy(templateBiometria + index, bytesReceived + uindx, 256);   // second 256 bytes

  /* for (int i = 0; i < 512; ++i) {
    printHex(templateBiometria[i], 2);
  }
  Serial.println("\nTemplate extraído"); */

  String hex = montarHex(templateBiometria);

  return hex;
}

String montarHex(uint8_t *templateBiometria) {
  String resultado = "";
  // Dica de ouro para o ESP32: reserva o espaço antes para ser 10x mais rápido
  resultado.reserve(1025); 

  for (int i = 0; i < 512; i++) {
    char tmp[3];
    sprintf(tmp, "%02X", templateBiometria[i]);
    resultado += tmp; // Vai "colando" um hex no outro
  }
  
  return resultado; // Retorna a string gigante pronta
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);

  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void setModoCadastroTrue(){
  if(sensorOcupado){
    Serial.println("Sensor ocupado no momento");
    Serial.println("Modo de cadastro desabilitado");
    enviarDadosCadastroBiometriaMqtt(0, "0");
    return;
  }
  modoCadastroBiometria = true;
  sensorOcupado = true;
}

void setModoCadastroFalse(){
  modoCadastroBiometria = false;
  sensorOcupado = false;
}