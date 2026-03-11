void setModoCadastroTrue(){
  if(sensorOcupado){
    Serial.println("Sensor ocupado no momento");
    Serial.println("Modo de cadastro desabilitado");
    enviarDadosCadastroBiometriaMqtt(1);
    return;
  }
  modoCadastroBiometria = true;
  sensorOcupado = true;
  finger.LEDcontrol(true);
}

void CadastrarBiometria(){
  int idSensor = buscarIdVazio();  

  if(idSensor == -1){
    Serial.print("Todos os slots da memória do sensor estão ocupados");
    setModoCadastroFalse();
    enviarDadosCadastroBiometriaMqtt(2);
    return;
  };

  uint8_t tentativasLerDigitaisMaximas = 3;
  uint8_t tentativasLerDigitais = 0;
  bool sucesso = false;   

  while(!sucesso && tentativasLerDigitais < tentativasLerDigitaisMaximas && modoCadastroBiometria){
    sucesso = lerDigitais(idSensor);

    if(!sucesso && !modoCadastroBiometria){
      Serial.println("Operação cancelada");
      enviarDadosCadastroBiometriaMqtt(3);
      setModoCadastroFalse();
      return;
    }

    if(!sucesso){
      tentativasLerDigitais++;
      Serial.print("Falha na tentativa "); 
      Serial.print(tentativasLerDigitais);
      Serial.print("/"); Serial.print(tentativasLerDigitaisMaximas);
      enviarDadosCadastroBiometriaMqttFeedback(9);
      //delay(500);
    } 
  }  

  if(!sucesso){
    Serial.println("Limite de tentativas atingido!");
    setModoCadastroFalse();
    // Envia erro para o .NET - linha em baixo
    enviarDadosCadastroBiometriaMqtt(4); //código para limite de tentativa - temporário
    return;
  }

  uint8_t bytesReceived[534];
  String templateBiometriaHex = ExtrairTemplateBiometria(idSensor, bytesReceived);

  enviarDadosCadastroBiometriaMqtt(idSensor, templateBiometriaHex);

  sucesso = false;
  unsigned long inicioCronometroSalvar;
  const long tempoLimiteSalvar = 10000;
  
  const long momentoAtual = millis();  
  const long tempoLimiteGeral = 30000;  
  
  while(!sucesso && (millis() - momentoAtual < tempoLimiteGeral)){
    clientMQTT.loop();    

    if(processarSalvarTemplate){
      inicioCronometroSalvar = millis();

      while(!sucesso && (millis() - inicioCronometroSalvar < tempoLimiteSalvar)){ 
        sucesso = salvarTemplate(idSensor, bytesReceived);
        yield();
      }

      // Se terminou o tempo interno e não teve sucesso, força a saída
      if(!sucesso) break; 
    }
    yield();
  }

  if(sucesso){
    enviarDadosCadastroBiometriaMqtt(idSensor, 0);
    enviarDadosCadastroBiometriaMqttFeedback(10);
  } 
  else {
    enviarDadosCadastroBiometriaMqtt(idSensor, 5); //erro na gravação do template
  }

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

bool lerDigitais(uint16_t id){
  int statusFingerprint = -1;
  
  Serial.print("Aguardando leitura de biometria do usuário no ID: "); Serial.println(id);
  enviarDadosCadastroBiometriaMqttFeedback(1);

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
      enviarDadosCadastroBiometriaMqttFeedback(2);
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
      enviarDadosCadastroBiometriaMqttFeedback(3);
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
  enviarDadosCadastroBiometriaMqttFeedback(4);
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
  enviarDadosCadastroBiometriaMqttFeedback(5);
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
        enviarDadosCadastroBiometriaMqttFeedback(6);
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
      enviarDadosCadastroBiometriaMqttFeedback(7);
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
    enviarDadosCadastroBiometriaMqttFeedback(8);
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

bool salvarTemplate(uint8_t idSensor, uint8_t *templateBiometria534){
  // 1. Comando DownChar (Aviso)
  uint8_t downCharCmd[] = {0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x00, 0x04, 0x09, 0x01, 0x00, 0x0F};
  
  // Limpa qualquer lixo que esteja no RX antes de começar
  while(mySerial.available()) mySerial.read();

  mySerial.write(downCharCmd, 13);
  delay(100); // Aumentei um pouco para o sensor "respirar"

  // 2. Limpa a resposta de "OK" que o sensor deu ao comando 0x09
  // Se não limpar, o ESP32 pode se confundir na próxima leitura
  while(mySerial.available()) mySerial.read();

  // 3. Envia o template em blocos (mais seguro para o buffer do AS608)
  for (int i = 0; i < 534; i++) {
    mySerial.write(templateBiometria534[i]);
    if (i % 32 == 0) delay(2); // Pequena pausa a cada 32 bytes
  }
  mySerial.flush();
  
  delay(150); // Tempo para o sensor processar o arquivo de caracteres

  int statusFingerprint = -1;
  Serial.print("ID "); Serial.println(idSensor);

  statusFingerprint = finger.storeModel(idSensor);
  if (statusFingerprint == FINGERPRINT_OK) {
    Serial.println("Stored!");    
    return true;
  } 
  else if (statusFingerprint == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
  } 
  else if (statusFingerprint == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
  } 
  else if (statusFingerprint == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
  } 
  else {
    Serial.println("Unknown error");
  }
  
  return false;
}

String ExtrairTemplateBiometria(int id, uint8_t *bytesReceived){
  // one data packet is 267 bytes. in one data packet, 11 bytes are 'usesless' :D
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

void setModoCadastroFalse(){
  modoCadastroBiometria = false;
  sensorOcupado = false;
  processarSalvarTemplate = false;
  finger.LEDcontrol(false);
}