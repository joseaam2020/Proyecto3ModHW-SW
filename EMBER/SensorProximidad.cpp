#include "SensorProximidad.h"

SensorProximidad::SensorProximidad(uint8_t trigPin, uint8_t echoPin, float umbralDeteccionCm)
  : trigPin(trigPin), echoPin(echoPin), distancia(-1.0f),
    umbralDeteccion(umbralDeteccionCm), activo(false) {
}

void SensorProximidad::init() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  activo = true;
}

// Lectura HC-SR04: pulso TRIG de 10us, mide el ancho del eco en ECHO.
// d == 0 (timeout) se reporta como -1 (sin objeto detectado).
float SensorProximidad::leer() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long d = pulseIn(echoPin, HIGH, 30000); // 30ms timeout (~5m max range)
  distancia = (d == 0) ? -1.0f : d * 0.034f / 2.0f;
  return distancia;
}

bool SensorProximidad::detectarPresencia() {
  float d = leer();
  return (d > 0) && (d <= umbralDeteccion);
}
