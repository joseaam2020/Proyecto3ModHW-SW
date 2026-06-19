#include "Racha.h"
#include "Config.h"

Racha::Racha()
  : diasConsecutivos(0), fechaInicio(0), fechaUltimaInteraccion(0), activa(false) {
  hitos[0] = Hito(HITO_DIAS_1, "Primera semana");
  hitos[1] = Hito(HITO_DIAS_2, "Un mes");
  hitos[2] = Hito(HITO_DIAS_3, "Tres meses");
}

void Racha::incrementar() {
  if (diasConsecutivos == 0) {
    fechaInicio = time(nullptr);
  }
  diasConsecutivos++;
  fechaUltimaInteraccion = time(nullptr);
  activa = true;
}

void Racha::reiniciar() {
  diasConsecutivos = 0;
  fechaInicio = 0;
  activa = false;
  // fechaUltimaInteraccion se conserva como registro de la última
  // interacción antes de perder la racha.
}

Hito* Racha::verificarHito() {
  for (int i = 0; i < 3; i++) {
    if (!hitos[i].isCelebrado() && hitos[i].estaAlcanzado(diasConsecutivos)) {
      hitos[i].marcarCelebrado();
      return &hitos[i];
    }
  }
  return nullptr;
}

bool Racha::estaActiva() const {
  return activa;
}

int Racha::getDiasConsecutivos() const {
  return diasConsecutivos;
}

time_t Racha::getFechaInicio() const {
  return fechaInicio;
}

time_t Racha::getFechaUltimaInteraccion() const {
  return fechaUltimaInteraccion;
}
