#include <SoftwareSerial.h>
#include <LoRa_E32.h>

#define PIN_RX 5
#define PIN_TX 3
#define PIN_M0 2
#define PIN_M1 4
#define PIN_AX 6
#define LED_PIN 8

SoftwareSerial mySerial(PIN_TX, PIN_RX);
LoRa_E32 e32ttl100(&mySerial, PIN_AX, PIN_M0, PIN_M1);

// Controle
unsigned long ultimoContadorRecebido = 0;
unsigned long pacotesRecebidos = 0;
unsigned long pacotesPerdidos = 0;
unsigned long tempoDeteccaoMovimento = 0;

// Horário manual configurado
unsigned long horarioInicial = (14UL * 3600UL) + (0UL * 60UL) + 0UL; // 14:00:00
unsigned long tempoInicialMillis;

void setup() {
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(PIN_M0, LOW);
  digitalWrite(PIN_M1, LOW);

  Serial.begin(9600);
  delay(500);

  e32ttl100.begin();
  Serial.println("Receptor LoRa iniciado...");

  ResponseStructContainer c;
  c = e32ttl100.getConfiguration();
  Configuration configuration = *(Configuration*) c.data;
  Serial.println(c.status.getResponseDescription());

  configuration.ADDL = 0x03;
  configuration.ADDH = 0x00;
  configuration.CHAN = 0x04;
  configuration.SPED.uartBaudRate = UART_BPS_9600;
  configuration.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;

  e32ttl100.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  printParameters(configuration);

  ResponseStructContainer cMi;
  cMi = e32ttl100.getModuleInformation();
  ModuleInformation mi = *(ModuleInformation*)cMi.data;

  Serial.println(cMi.status.getResponseDescription());
  printModuleInformation(mi);

  c.close();

  tempoInicialMillis = millis();
}

void loop() {
  if (e32ttl100.available() > 1) {
    ResponseContainer rc = e32ttl100.receiveMessage();
    String mensagem = rc.data;

    Serial.print("Recebido: ");
    Serial.println(mensagem.substring(0, mensagem.lastIndexOf(',')));

    // Formato esperado: contador;ldr,pir,timestamp
    int separador1 = mensagem.indexOf(';');
    int separador2 = mensagem.indexOf(',');
    int separador3 = mensagem.lastIndexOf(',');

    if (separador1 > 0 && separador2 > separador1 && separador3 > separador2) {
      String contadorStr = mensagem.substring(0, separador1);
      String ldrStr = mensagem.substring(separador1 + 1, separador2);
      String pirStr = mensagem.substring(separador2 + 1, separador3);
      String tempoTransmissorStr = mensagem.substring(separador3 + 1);

      unsigned long contadorAtual = contadorStr.toInt();
      int ldrValue = ldrStr.toInt();
      int pirValue = pirStr.toInt();
      unsigned long tempoTransmissor = tempoTransmissorStr.toInt();

      // Tempo local do receptor
      unsigned long tempoReceptor = horarioInicial + ((millis() - tempoInicialMillis) / 1000);

      // Calcula diferença
      long diferenca = (long)tempoReceptor - (long)tempoTransmissor;

      // Verificar pacotes perdidos
      if (pacotesRecebidos > 0 && contadorAtual > ultimoContadorRecebido + 1) {
        pacotesPerdidos += (contadorAtual - ultimoContadorRecebido - 1);
      }

      ultimoContadorRecebido = contadorAtual;
      pacotesRecebidos++;

      Serial.println("Contador do transmissor: " + String(contadorAtual));
      Serial.println("LDR: " + String(ldrValue) + " | PIR: " + String(pirValue));
      //Serial.println("Timestamp do Transmissor: " + String(tempoTransmissor) + " s");
      //Serial.println("Timestamp do Receptor   : " + String(tempoReceptor) + " s");
      //Serial.println("Diferença de tempo      : " + String(diferenca) + " s");
      Serial.println("Total recebidos: " + String(pacotesRecebidos));
      Serial.println("Perdidos: " + String(pacotesPerdidos));

      if (pirValue == 1) {
        tempoDeteccaoMovimento = millis();
      }

      if (ldrValue <= 300 && tempoDeteccaoMovimento > 0) {
        unsigned long tempoResposta = millis() - tempoDeteccaoMovimento;
        Serial.println("⏱️ Tempo de resposta LDR após movimento: " + String(tempoResposta) + " ms");
        tempoDeteccaoMovimento = 0;
      }

      if (pirValue == 1 && ldrValue <= 300) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }

      // ENVIO PARA PROCESSING
      Serial.print(ldrValue);
      Serial.print(",");
      Serial.println(pirValue);

      Serial.println("-------------------------------------------------");
    }
  }
}

void printParameters(struct Configuration configuration) {
  Serial.println("----------------------------------------");

  Serial.print(F("HEAD BIN: "));
  Serial.print(configuration.HEAD, BIN); Serial.print(" ");
  Serial.print(configuration.HEAD, DEC); Serial.print(" ");
  Serial.println(configuration.HEAD, HEX);

  Serial.println(F(" "));
  Serial.print(F("AddH BIN: "));
  Serial.println(configuration.ADDH, BIN);
  Serial.print(F("AddL BIN: "));
  Serial.println(configuration.ADDL, BIN);
  Serial.print(F("Chan BIN: "));
  Serial.print(configuration.CHAN, DEC); Serial.print(" -> ");
  Serial.println(configuration.getChannelDescription());

  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit BIN    : "));
  Serial.print(configuration.SPED.uartParity, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTParityDescription());

  Serial.print(F("SpeedUARTDataRate BIN : "));
  Serial.print(configuration.SPED.uartBaudRate, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTBaudRate());

  Serial.print(F("SpeedAirDataRate BIN  : "));
  Serial.print(configuration.SPED.airDataRate, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getAirDataRate());

  Serial.print(F("OptionTrans BIN       : "));
  Serial.print(configuration.OPTION.fixedTransmission, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFixedTransmissionDescription());

  Serial.print(F("OptionPullup BIN      : "));
  Serial.print(configuration.OPTION.ioDriveMode, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getIODroveModeDescription());

  Serial.print(F("OptionWakeup BIN      : "));
  Serial.print(configuration.OPTION.wirelessWakeupTime, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getWirelessWakeUPTimeDescription());

  Serial.print(F("OptionFEC BIN         : "));
  Serial.print(configuration.OPTION.fec, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFECDescription());

  Serial.print(F("OptionPower BIN       : "));
  Serial.print(configuration.OPTION.transmissionPower, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getTransmissionPowerDescription());

  Serial.println("----------------------------------------");
}

void printModuleInformation(struct ModuleInformation moduleInformation) {
  Serial.println("----------------------------------------");
  Serial.print(F("HEAD BIN: "));
  Serial.print(moduleInformation.HEAD, BIN); Serial.print(" ");
  Serial.print(moduleInformation.HEAD, DEC); Serial.print(" ");
  Serial.println(moduleInformation.HEAD, HEX);

  Serial.print(F("Freq.: "));
  Serial.println(moduleInformation.frequency, HEX);
  Serial.print(F("Version  : "));
  Serial.println(moduleInformation.version, HEX);
  Serial.print(F("Features : "));
  Serial.println(moduleInformation.features, HEX);
  Serial.println("----------------------------------------");
}
