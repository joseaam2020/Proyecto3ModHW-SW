#ifndef HITO_H
#define HITO_H

#include <Arduino.h>
#include <time.h>

class Hito {
  public:
    Hito();
    Hito(int diasRequeridos, const String &descripcion);

    bool estaAlcanzado(int dias) const;
    void marcarCelebrado();

    int getDiasRequeridos() const;
    String getDescripcion() const;
    bool isCelebrado() const;
    time_t getFechaAlcanzado() const;

  private:
    int diasRequeridos;
    String descripcion;
    time_t fechaAlcanzado;
    bool celebrado;
};

#endif
