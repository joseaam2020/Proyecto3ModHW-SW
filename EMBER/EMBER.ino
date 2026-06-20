/* EMBER — Tótem interactivo de continuidad y sobriedad
 * Firmware principal del ESP32: máquina de estados, sensores, actuadores,
 * racha/hitos, persistencia en flash y reloj local. La sincronización
 * con el backend (Wi-Fi/HTTP) llega en la Etapa 11.
 *
 * Ver EMBER/Config.h para activar/desactivar MODO_DEMO (comprime los
 * tiempos de espera de 24h a segundos para poder ver el ciclo completo
 * Reposo → Riesgo → Reinicio en una demo corta).
 */

#include "SystemState.h"
#include "Config.h"
#include "Totem.h"

Totem totem("EMBER-01");

void setup() {
  Serial.begin(115200);
  delay(1000); // tiempo para abrir el monitor serial antes del primer print
  Serial.println("EMBER firmware starting...");
  totem.encender();
}

void loop() {
  totem.actualizar();
}
