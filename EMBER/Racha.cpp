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

  // Los hitos son logros de LA racha actual, no de toda la vida del
  // tótem: sin esto, una vez celebrado un hito (p. ej. 7 días) quedaba
  // marcado para siempre, y una racha nueva que volviera a llegar a 7
  // días jamás volvía a disparar la Celebración.
  for (int i = 0; i < 3; i++) {
    hitos[i].restaurar(false, 0);
  }
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

void Racha::restaurar(int dias, time_t inicio, time_t ultimaInteraccion, bool activa) {
  diasConsecutivos = dias;
  fechaInicio = inicio;
  fechaUltimaInteraccion = ultimaInteraccion;
  this->activa = activa;
}

Hito& Racha::getHito(int indice) {
  return hitos[indice];
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
