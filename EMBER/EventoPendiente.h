#ifndef EVENTO_PENDIENTE_H
#define EVENTO_PENDIENTE_H

#include <time.h>

enum TipoEventoSync {
  EVT_INTERACCION,
  EVT_HITO,
  EVT_REINICIO
};

struct EventoPendiente {
  TipoEventoSync tipo;
  time_t timestamp;
};

#endif
