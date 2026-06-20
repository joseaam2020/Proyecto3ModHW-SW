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

    // Para Almacenamiento: restaura el estado tal como se persistió,
    // sin usar la hora actual (a diferencia de marcarCelebrado()).
    void restaurar(bool celebrado, time_t fechaAlcanzado);

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
