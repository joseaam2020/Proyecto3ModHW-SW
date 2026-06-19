#ifndef SENSOR_TACTIL_H
#define SENSOR_TACTIL_H

#include <Arduino.h>

class SensorTactil {
  public:
    SensorTactil(uint8_t ioPin, float sensibilidad);

    void init();
    bool leer();
    unsigned long registrarContacto();

    unsigned long getUltimoContacto() const;

  private:
    uint8_t ioPin;
    bool tocado;
    float sensibilidad;
    unsigned long ultimoContacto;
};

#endif
