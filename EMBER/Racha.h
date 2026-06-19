#ifndef RACHA_H
#define RACHA_H

#include <Arduino.h>
#include <time.h>
#include "Hito.h"

class Racha {
  public:
    Racha();

    // Acción mecánica: suma un día. La decisión de SI debe llamarse
    // (p. ej. no duplicar si ya se interactuó hoy) es responsabilidad
    // de Totem, no de esta clase.
    void incrementar();

    // Vuelve la racha a cero (Día 1 lo establece la próxima incrementar()).
    void reiniciar();

    // Recorre los hitos fijos (7/30/90 días) y marca como celebrado el
    // primero que se haya alcanzado y no estuviera celebrado todavía.
    // Devuelve nullptr si no hay un hito nuevo que celebrar.
    Hito* verificarHito();

    bool estaActiva() const;
    int getDiasConsecutivos() const;
    time_t getFechaInicio() const;
    time_t getFechaUltimaInteraccion() const;

    // Para Almacenamiento: restaura el estado cargado desde flash.
    void restaurar(int dias, time_t inicio, time_t ultimaInteraccion, bool activa);
    Hito& getHito(int indice); // indice 0-2

  private:
    int diasConsecutivos;
    time_t fechaInicio;
    time_t fechaUltimaInteraccion;
    bool activa;

    Hito hitos[3];
};

#endif
