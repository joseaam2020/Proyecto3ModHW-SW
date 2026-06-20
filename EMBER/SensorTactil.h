#ifndef SENSOR_TACTIL_H
#define SENSOR_TACTIL_H

#include <Arduino.h>

class SensorTactil {
  public:
    SensorTactil(uint8_t ioPin, float sensibilidad);

    void init();
    bool leer();

    // leer() es una lectura de nivel (pin tocado en este instante): si el
    // dedo se queda apoyado, sigue devolviendo true loop tras loop. Para
    // disparar un registro de contacto se necesita el FLANCO de un toque
    // nuevo (pasó de no-tocado a tocado), no que esté tocado de forma
    // continua — si no, un dedo apoyado durante el cambio de "día" cuenta
    // como un toque nuevo sin que el usuario haya vuelto a tocar.
    bool detectarToqueNuevo();

    unsigned long registrarContacto();

    unsigned long getUltimoContacto() const;

  private:
    uint8_t ioPin;
    bool tocado;
    bool tocadoAnterior;
    float sensibilidad;
    unsigned long ultimoContacto;
};

#endif
