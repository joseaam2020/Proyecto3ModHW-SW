#ifndef MOTOR_VIBRADOR_H
#define MOTOR_VIBRADOR_H

#include <Arduino.h>
#include "SystemState.h"

class MotorVibrador {
  public:
    MotorVibrador(uint8_t pin);

    void init();
    void setIntensidad(float v); // 0.0 - 1.0
    void vibrar(unsigned long ms);

    // Aplica el patrón de vibración asociado a un estado del sistema.
    // Para RIESGO_INACTIVIDAD, debe llamarse repetidamente desde loop()
    // mientras el tótem permanezca en ese estado: dispara un recordatorio
    // de DURACION_RECORDATORIO_RIESGO_MS cada INTERVALO_RECORDATORIO_RIESGO_MS
    // (el primer llamado dispara de inmediato).
    void aplicarEstado(SystemState estado);

    // Debe llamarse en cada loop() para apagar el motor cuando
    // termina la duración solicitada (no bloqueante).
    void actualizar();

  private:
    uint8_t pin;
    float intensidad;
    unsigned int duracion;
    String patron;

    bool activo;
    unsigned long inicioMs;

    // Último disparo del recordatorio periódico en RIESGO_INACTIVIDAD.
    unsigned long ultimoRecordatorioMs;
};

#endif
