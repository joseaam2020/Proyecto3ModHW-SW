#ifndef COMUNICACION_APP_H
#define COMUNICACION_APP_H

#include <Arduino.h>

// Cliente HTTP del backend Flask (Etapa 11). El tótem es la fuente de
// verdad de la racha: este componente solo transporta datos, no decide
// nada — Totem es quien llama a estos métodos y reacciona al resultado.
class ComunicacionApp {
  public:
    // Inicia la conexión WiFi (no bloqueante: si no hay red disponible,
    // el tótem sigue funcionando normalmente sin conexión).
    void init();

    bool estaConectado();

    // Hora configurada desde la app, ya proyectada al momento actual
    // por el backend (ver POST/GET /config/hora) — devuelve false si no
    // hay conexión o el backend todavía no tiene una hora configurada.
    bool obtenerHora(uint8_t &horas, uint8_t &minutos);

    // Reporta la racha real del tótem a /device/sync. "evento" es
    // "checkin" | "hito" | "reinicio" (ver TipoEventoSync). Devuelve
    // false si la llamada HTTP falla, para que Totem decida reintentar.
    bool reportarEstado(int dias, const char *estadoVisual, const char *evento);

    // Borrado de memoria solicitado desde el botón "Borrar memoria del
    // tótem" en Perfil (ver /device/wipe). false si no hay conexión.
    bool hayBorradoPendiente();
    void confirmarBorrado();
};

#endif
