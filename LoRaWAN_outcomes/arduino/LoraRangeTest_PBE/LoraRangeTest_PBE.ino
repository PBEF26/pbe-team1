/*
  LoraRangeTest_PBE.ino  -  PBE Team 1, Bloque B1, Task 6 (range test)
  Basado en el ejemplo LoraSendAndReceive de la libreria MKRWAN.

  Comandos por el Monitor Serie (configurad "Nueva linea" y 9600 baudios):
    SF7 ... SF12  -> elige el spreading factor (se aplica antes de cada envio)
    N<num>        -> envia un payload de prueba de <num> bytes (ej: N5, N50)
    cualquier otro texto -> se envia tal cual

  Los uplinks son NO confirmados: el Sandbox de TTN solo permite unos
  10 downlinks al dia por dispositivo, y cada ACK gasta uno.
  Si un paquete se ha recibido o perdido se comprueba en Live Data de TTS.
*/

#include <MKRWAN.h>
#include "arduino_secrets.h"   // SECRET_APP_EUI y SECRET_APP_KEY (no subir a GitHub)

LoRaModem modem;

String appEui = SECRET_APP_EUI;
String appKey = SECRET_APP_KEY;

int msgCount = 0;
int currentSF = 7;

// En EU868: DR0 = SF12 ... DR5 = SF7
int sfToDataRate(int sf) {
  return 12 - sf;
}

void setSF(int sf) {
  if (sf < 7 || sf > 12) {
    Serial.println("SF no valido (7-12)");
    return;
  }
  if (modem.dataRate(sfToDataRate(sf))) {
    currentSF = sf;
    Serial.print("SF fijado a SF");
    Serial.print(sf);
    Serial.print(" (DR");
    Serial.print(sfToDataRate(sf));
    Serial.println(")");
  } else {
    Serial.println("Error al cambiar el data rate");
  }
}

void sendPayload(const String &payload) {
  msgCount++;
  Serial.print("MSG ");
  Serial.print(msgCount);
  Serial.print(" | SF");
  Serial.print(currentSF);
  Serial.print(" | ");
  Serial.print(payload.length());
  Serial.print(" B | t=");
  Serial.print(millis() / 1000);
  Serial.print(" s | ");
  Serial.println(payload);

  modem.dataRate(sfToDataRate(currentSF));
  Serial.print("  -> DR configurado en el modulo: ");
  Serial.println(modem.getDataRate());
  modem.beginPacket();
  
  modem.print(payload);
  int err = modem.endPacket(false);   // false = no confirmado

  if (err > 0) {
    Serial.println("  -> enviado al modulo LoRa");
  } else {
    Serial.println("  -> ERROR al enviar (payload demasiado grande para este SF o duty cycle)");
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  if (!modem.begin(EU868)) {
    Serial.println("No se pudo iniciar el modulo LoRa");
    while (1) {}
  }
  Serial.print("Version modulo: ");
  Serial.println(modem.version());
  Serial.print("DevEUI: ");
  Serial.println(modem.deviceEUI());

  if (!modem.joinOTAA(appEui, appKey)) {
    Serial.println("Join fallido. Acercaos al gateway y reiniciad la placa.");
    while (1) {}
  }
  Serial.println("Join correcto");

  modem.setADR(true);   // ADR activado: con ADR desactivado el modulo transmitia a SF12 aunque se pidiera DR5
  modem.setPort(2);      // mismo FPort de uplink que en las pruebas anteriores
  setSF(7);

  Serial.println("Listo. Comandos: SF7..SF12, N<bytes>, o texto libre.");
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0) return;

    if (input.startsWith("SF")) {
      setSF(input.substring(2).toInt());
    } else if (input.startsWith("N") && input.substring(1).toInt() > 0) {
      int n = input.substring(1).toInt();
      String payload = "";
      for (int i = 0; i < n; i++) {
        payload += char('A' + (i % 26));   // ABCDEF... de longitud n
      }
      sendPayload(payload);
    } else {
      sendPayload(input);
    }
  }

  // Mostrar un downlink si llega alguno (no se espera en esta prueba)
  if (modem.available()) {
    Serial.print("Downlink recibido: ");
    while (modem.available()) {
      Serial.print((char)modem.read());
    }
    Serial.println();
  }
}
