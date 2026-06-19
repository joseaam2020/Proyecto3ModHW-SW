#include "Hito.h"

Hito::Hito()
  : diasRequeridos(0), descripcion(""), fechaAlcanzado(0), celebrado(false) {
}

Hito::Hito(int diasRequeridos, const String &descripcion)
  : diasRequeridos(diasRequeridos), descripcion(descripcion),
    fechaAlcanzado(0), celebrado(false) {
}

bool Hito::estaAlcanzado(int dias) const {
  return dias >= diasRequeridos;
}

void Hito::marcarCelebrado() {
  celebrado = true;
  fechaAlcanzado = time(nullptr);
}

int Hito::getDiasRequeridos() const {
  return diasRequeridos;
}

String Hito::getDescripcion() const {
  return descripcion;
}

bool Hito::isCelebrado() const {
  return celebrado;
}

time_t Hito::getFechaAlcanzado() const {
  return fechaAlcanzado;
}
