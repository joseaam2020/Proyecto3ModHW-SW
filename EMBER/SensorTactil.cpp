#include "SensorTactil.h"

SensorTactil::SensorTactil(uint8_t ioPin, float sensibilidad)
  : ioPin(ioPin), tocado(false), tocadoAnterior(false),
    sensibilidad(sensibilidad), ultimoContacto(0) {
}

void SensorTactil::init() {
  pinMode(ioPin, INPUT);
}

bool SensorTactil::leer() {
  tocado = digitalRead(ioPin);
  return tocado;
}

bool SensorTactil::detectarToqueNuevo() {
  bool tocadoAhora = leer();
  bool esNuevo = tocadoAhora && !tocadoAnterior;
  tocadoAnterior = tocadoAhora;
  return esNuevo;
}

// Marca el contacto actual y devuelve el timestamp registrado.
unsigned long SensorTactil::registrarContacto() {
  tocado = true;
  ultimoContacto = millis();
  return ultimoContacto;
}

unsigned long SensorTactil::getUltimoContacto() const {
  return ultimoContacto;
}
