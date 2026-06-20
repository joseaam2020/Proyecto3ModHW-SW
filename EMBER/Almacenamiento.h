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

    // Última hora real conocida (recibida del backend más adelante).
    // Se persiste para que un reinicio arranque desde ahí en vez de 00:00,
    // aunque se desactualice mientras no haya una nueva sincronización.
    void guardarHora(uint8_t horas, uint8_t minutos);
    void cargarHora(uint8_t &horas, uint8_t &minutos);

    // Cola de eventos no confirmados con la app (CU-02/CU-04, escenarios
    // de pérdida de conexión). Solo guarda los deltas, no toda la racha.
    void encolarEvento(TipoEventoSync tipo);
    int contarPendientes();
    bool obtenerPendiente(int indice, EventoPendiente &out);
    void eliminarPendiente(int indice); // quita uno ya confirmado, conserva el orden
    void limpiarPendientes();

    // Borrado completo de flash (racha, hitos, hora, cola de pendientes)
    // — solicitado desde la app (botón "Borrar memoria" en Perfil), no
    // forma parte del flujo normal de pérdida de racha (eso es
    // Racha::reiniciar(), que no toca flash directamente).
    void borrarTodo();

  private:
    Preferences prefs;
    static const int MAX_PENDIENTES = 16;

    void guardarHito(int indice, Hito &hito);
    void cargarHito(int indice, bool &celebrado, time_t &fechaAlcanzado);
};

#endif
