#ifndef SENSOR_PROXIMIDAD_H
#define SENSOR_PROXIMIDAD_H

#include <Arduino.h>

class SensorProximidad {
  public:
    SensorProximidad(uint8_t trigPin, uint8_t echoPin, float umbralDeteccionCm);

    void init();
    float leer();
    bool detectarPresencia();

  private:
    uint8_t trigPin;
    uint8_t echoPin;
    float distancia;
    float umbralDeteccion;
    bool activo;
};

#endif
