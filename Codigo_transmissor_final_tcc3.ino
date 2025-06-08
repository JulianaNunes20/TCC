#include <SoftwareSerial.h>
#include <LoRa_E32.h>
#include "stdlib.h"
#include "string.h"

#define PIN_RX 5
#define PIN_TX 3
#define PIN_M0 2
#define PIN_M1 4
#define PIN_AX 6
#define LDR_PIN A0
#define PIR_PIN 7
#define LED_PIN 8

SoftwareSerial mySerial(PIN_TX, PIN_RX);
LoRa_E32 e32ttl100(&mySerial, PIN_AX, PIN_M0, PIN_M1);

int countSent = 1;
unsigned long pirDetectedTime = 0;

// Horário manual configurado (Exemplo: 14:00:00)
unsigned long horarioInicial = (14UL * 3600UL) + (0UL * 60UL) + 0UL; // em segundos
unsigned long tempoInicialMillis;

void printParameters(struct Configuration configuration);
void printModuleInformation(struct ModuleInformation moduleInformation);

void setup() {
  pinMode(PIN_M0, OUTPUT);
  pinMode(PIN_M1, OUTPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(PIN_M0, HIGH);
  digitalWrite(PIN_M1, HIGH);

  Serial.begin(9600);
  delay(500);

  e32ttl100.begin();

  ResponseStructContainer c = e32ttl100.getConfiguration();
  Configuration configuration = *(Configuration*) c.data;

  Serial.println(c.status.getResponseDescription());

  configuration.ADDL = 0x01;
  configuration.ADDH = 0x00;
  configuration.CHAN = 0x04;
  configuration.OPTION.fixedTransmission = FT_FIXED_TRANSMISSION;
  configuration.SPED.uartBaudRate = UART_BPS_9600;

  e32ttl100.setConfiguration(configuration, WRITE_CFG_PWR_DWN_SAVE);
  printParameters(configuration);

  ResponseStructContainer cMi = e32ttl100.getModuleInformation();
  ModuleInformation mi = *(ModuleInformation*)cMi.data;

  Serial.println(cMi.status.getResponseDescription());
  printModuleInformation(mi);

  c.close();

  digitalWrite(PIN_M0, LOW);
  digitalWrite(PIN_M1, LOW);
  delay(500);

  tempoInicialMillis = millis();
}

void loop() {
  int ldrValue = analogRead(LDR_PIN);
  int pirValue = digitalRead(PIR_PIN);

  // Registrar tempo de detecção do PIR
  if (pirValue == HIGH && pirDetectedTime == 0) {
    pirDetectedTime = millis();
  }

  // Mostrar tempo desde detecção
  if (pirDetectedTime > 0) {
    unsigned long elapsed = millis() - pirDetectedTime;
    Serial.print("Tempo desde PIR detectado: ");
    Serial.print(elapsed);
    Serial.println(" ms");
  }

  sendLoRa(ldrValue, pirValue);

  // Resetar tempo se PIR parar de detectar
  if (pirValue == LOW) {
    pirDetectedTime = 0;
  }

  // LED local para teste visual
  if (pirValue == HIGH && ldrValue <= 300) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  delay(2000);
}

void sendLoRa(int ldr, int pir) {
  unsigned long tempoAtual = horarioInicial + ((millis() - tempoInicialMillis) / 1000);

  String mensagem = String(countSent) + ";" + String(ldr) + "," + String(pir) + "," + String(tempoAtual);
  ResponseStatus rs = e32ttl100.sendFixedMessage(0, 3, 0x04, mensagem);

  Serial.println("Enviando via LoRa: " + mensagem);
  Serial.println(rs.getResponseDescription());

  countSent++;
}

void printParameters(struct Configuration configuration) {
  Serial.println("----------------------------------------");
  Serial.print(F("HEAD BIN: ")); Serial.print(configuration.HEAD, BIN); Serial.print(" ");
  Serial.print(configuration.HEAD, DEC); Serial.print(" "); Serial.println(configuration.HEAD, HEX);

  Serial.println(F(" "));
  Serial.print(F("AddH BIN: ")); Serial.println(configuration.ADDH, BIN);
  Serial.print(F("AddL BIN: ")); Serial.println(configuration.ADDL, BIN);
  Serial.print(F("Chan BIN: ")); Serial.print(configuration.CHAN, DEC); Serial.print(" -> ");
  Serial.println(configuration.getChannelDescription());

  Serial.println(F(" "));
  Serial.print(F("SpeedParityBit BIN    : ")); Serial.print(configuration.SPED.uartParity, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTParityDescription());

  Serial.print(F("SpeedUARTDataRate BIN : ")); Serial.print(configuration.SPED.uartBaudRate, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getUARTBaudRate());

  Serial.print(F("SpeedAirDataRate BIN  : ")); Serial.print(configuration.SPED.airDataRate, BIN); Serial.print(" -> ");
  Serial.println(configuration.SPED.getAirDataRate());

  Serial.print(F("OptionTrans BIN       : ")); Serial.print(configuration.OPTION.fixedTransmission, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFixedTransmissionDescription());

  Serial.print(F("OptionPullup BIN      : ")); Serial.print(configuration.OPTION.ioDriveMode, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getIODroveModeDescription());

  Serial.print(F("OptionWakeup BIN      : ")); Serial.print(configuration.OPTION.wirelessWakeupTime, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getWirelessWakeUPTimeDescription());

  Serial.print(F("OptionFEC BIN         : ")); Serial.print(configuration.OPTION.fec, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getFECDescription());

  Serial.print(F("OptionPower BIN       : ")); Serial.print(configuration.OPTION.transmissionPower, BIN); Serial.print(" -> ");
  Serial.println(configuration.OPTION.getTransmissionPowerDescription());

  Serial.println("----------------------------------------");
}

void printModuleInformation(struct ModuleInformation moduleInformation) {
  Serial.println("----------------------------------------");
  Serial.print(F("HEAD BIN: ")); Serial.print(moduleInformation.HEAD, BIN); Serial.print(" ");
  Serial.print(moduleInformation.HEAD, DEC); Serial.print(" "); Serial.println(moduleInformation.HEAD, HEX);

  Serial.print(F("Freq.: ")); Serial.println(moduleInformation.frequency, HEX);
  Serial.print(F("Version  : ")); Serial.println(moduleInformation.version, HEX);
  Serial.print(F("Features : ")); Serial.println(moduleInformation.features, HEX);
  Serial.println("----------------------------------------");
}
