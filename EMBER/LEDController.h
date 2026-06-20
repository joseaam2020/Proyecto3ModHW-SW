#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Arduino.h>
#include "SystemState.h"

class LEDController {
  public:
    LEDController(uint8_t rPin, uint8_t gPin, uint8_t bPin);

    void init();
    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setIntensidad(float v); // 0.0 - 1.0

    // Aplica el color/patrón asociado a un estado del sistema.
    // Para CELEBRACION, debe llamarse repetidamente desde loop()
    // durante DURACION_CELEBRACION_MS para animar la secuencia.
    void aplicarEstado(SystemState estado);

  private:
    uint8_t rPin, gPin, bPin;
    uint8_t colorActual[3];
    float intensidad;
    String patron;

    void write(uint8_t r, uint8_t g, uint8_t b);
};

#endif
