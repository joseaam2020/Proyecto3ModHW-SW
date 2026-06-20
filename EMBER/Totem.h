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
#include "Almacenamiento.h"
#include "EventoPendiente.h"

class Totem {
  public:
    Totem(const String &id);

    void encender();

    // Pensados para llamarse desde loop() en los estados donde aplican.
    void detectarProximidad();
    void registrarContacto();

    void cambiarEstado(SystemState e);

    // Fija la hora "real" (recibida del backend en la Etapa 11). A partir
    // de aquí, el reloj mostrado es horas:minutos + lo transcurrido desde
    // esta llamada — no vuelve a depender de millis()-desde-boot.
    void establecerHora(uint8_t horas, uint8_t minutos);

    // Placeholder: el envío real por Wi-Fi/HTTP llega en la Etapa 11.
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
    Almacenamiento almacenamiento;

    unsigned long tiempoEntradaEstado;
    unsigned long ultimoRefrescoReloj;
    unsigned long ultimaLecturaProximidadMs;

    // Horario fijo de "día" (ver DURACION_DIA_MS): inicioDiaMs marca
    // cuándo empezó el día actual; yaInteractuoHoy bloquea un segundo
    // registro hasta que el día termine, sin importar cuándo se tocó.
    unsigned long inicioDiaMs;
    bool yaInteractuoHoy;

    // Plazo de Riesgo/Inactividad: NO es una duración fija propia, es lo
    // que queda del día actual (inicioDiaMs + DURACION_DIA_MS) en el
    // momento de entrar a Riesgo. Se calcula una sola vez al entrar y no
    // se reajusta con el horario de "día" — si se reajustara, el plazo
    // nunca terminaría de cumplirse.
    unsigned long limiteRiesgoMs;

    // Hora base conocida (última recibida del backend, o la persistida de
    // un encendido anterior) + el millis() en que se fijó. El reloj actual
    // se calcula sumando los minutos transcurridos desde horaBaseMillis.
    uint8_t horaBaseHoras;
    uint8_t horaBaseMinutos;
    unsigned long horaBaseMillis;

    // Calcula horas:minutos actuales a partir de la hora base + lo
    // transcurrido, y los envía a PantallaEInk.
    void refrescarReloj();

    // Encola un evento si no hay conexión con la app (CU-02/CU-04, alt
    // de pérdida de conexión). El envío inmediato cuando sí hay conexión
    // se conecta en la Etapa 8.
    void registrarEventoPendiente(TipoEventoSync tipo);
};

#endif
