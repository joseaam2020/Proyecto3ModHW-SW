#ifndef ALMACENAMIENTO_H
#define ALMACENAMIENTO_H

#include <Arduino.h>
#include <Preferences.h>
#include "Racha.h"
#include "EventoPendiente.h"

// Encapsula el acceso a flash (NVS vía Preferences). Corresponde al
// componente "Memoria Flash (ESP32)" del diagrama de arquitectura,
// usado por Totem.
class Almacenamiento {
  public:
    void init();

    void guardarRacha(Racha &racha);
    void cargarRacha(Racha &racha);

    // Cola de eventos no confirmados con la app (CU-02/CU-04, escenarios
    // de pérdida de conexión). Solo guarda los deltas, no toda la racha.
    void encolarEvento(TipoEventoSync tipo);
    int contarPendientes();
    bool obtenerPendiente(int indice, EventoPendiente &out);
    void eliminarPendiente(int indice); // quita uno ya confirmado, conserva el orden
    void limpiarPendientes();

  private:
    Preferences prefs;
    static const int MAX_PENDIENTES = 16;

    void guardarHito(int indice, Hito &hito);
    void cargarHito(int indice, bool &celebrado, time_t &fechaAlcanzado);
};

#endif
