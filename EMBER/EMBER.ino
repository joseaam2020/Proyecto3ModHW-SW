/* EMBER — Tótem interactivo de continuidad y sobriedad
 * Firmware principal del ESP32. La lógica de sensores, actuadores,
 * racha/hitos y la máquina de estados se incorpora en etapas siguientes.
 */

#include "SystemState.h"
#include "Config.h"

void setup() {
  Serial.begin(115200);
  Serial.println("EMBER firmware starting...");
}

void loop() {
}
