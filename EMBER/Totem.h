#ifndef TOTEM_H
#define TOTEM_H

#include <Arduino.h>
#include <time.h>
#include "SystemState.h"
#include "SensorProximidad.h"
#include "SensorTactil.h"
#include "LEDController.h"
#include "MotorVibrador.h"
#include "PantallaEInk.h"
#include "Racha.h"

class Totem {
  public:
    Totem(const String &id);

    void encender();

    // Pensados para llamarse desde loop() en los estados donde aplican.
    void detectarProximidad();
    void registrarContacto();

    void cambiarEstado(SystemState e);

    // Placeholder: el envío real por BLE llega en la Etapa 8.
    void sincronizarConApp();

    // Revisa los timeouts de la máquina de estados y dispara las
    // transiciones automáticas correspondientes.
    void verificarInactividad();

    // No está en el diagrama de clases: punto de entrada único que
    // EMBER.ino llama en cada loop() para orquestar todo lo anterior.
    void actualizar();

    SystemState getEstadoActual() const;

  private:
    String id;
    SystemState estadoActual;
    bool conectado;
    time_t ultimaInteraccion;
    unsigned long umbralInactividad; // ms

    SensorProximidad sensorProximidad;
    SensorTactil sensorTactil;
    LEDController led;
    MotorVibrador motor;
    PantallaEInk pantalla;
    Racha racha;

    unsigned long tiempoEntradaEstado;
    unsigned long ultimoRefrescoReloj;

    // Hora "de relleno" derivada de millis() hasta que la Etapa 7/8
    // aporten una fuente de tiempo real.
    void refrescarRelojPlaceholder();
};

#endif
