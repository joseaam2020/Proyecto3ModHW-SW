// Pruebas de lógica pura para Racha e Hito. Corren en host (sin Arduino
// IDE ni ESP32), compilando directamente los .cpp reales de EMBER/.
//
// Compilar y correr:
//   g++ -std=c++11 -I stubs -I ../EMBER test_racha_hito.cpp \
//       ../EMBER/Racha.cpp ../EMBER/Hito.cpp -o test_racha_hito
//   ./test_racha_hito

#include <cstdio>
#include "../EMBER/Hito.h"
#include "../EMBER/Racha.h"

static int pruebasTotal = 0;
static int pruebasFallidas = 0;

#define ASSERT(cond, msg)                                          \
  do {                                                              \
    pruebasTotal++;                                                 \
    if (!(cond)) {                                                  \
      pruebasFallidas++;                                            \
      printf("FALLO: %s (linea %d)\n", msg, __LINE__);              \
    }                                                                \
  } while (0)

void test_hito_estaAlcanzado() {
  Hito h(7, "Primera semana");
  ASSERT(!h.estaAlcanzado(6), "estaAlcanzado(6) debe ser falso con diasRequeridos=7");
  ASSERT(h.estaAlcanzado(7), "estaAlcanzado(7) debe ser verdadero con diasRequeridos=7");
  ASSERT(h.estaAlcanzado(10), "estaAlcanzado(10) debe ser verdadero con diasRequeridos=7");
}

void test_hito_marcarCelebrado() {
  Hito h(7, "Primera semana");
  ASSERT(!h.isCelebrado(), "un hito nuevo no debe estar celebrado");
  ASSERT(h.getFechaAlcanzado() == 0, "fechaAlcanzado debe ser 0 antes de celebrar");

  h.marcarCelebrado();
  ASSERT(h.isCelebrado(), "marcarCelebrado() debe dejar celebrado=true");
  ASSERT(h.getFechaAlcanzado() != 0, "marcarCelebrado() debe fijar fechaAlcanzado a 'ahora'");
}

void test_hito_restaurar() {
  Hito h(30, "Un mes");
  h.restaurar(true, 123456);
  ASSERT(h.isCelebrado(), "restaurar(true, ...) debe dejar celebrado=true");
  ASSERT(h.getFechaAlcanzado() == 123456, "restaurar() debe fijar la fecha exacta dada, no 'ahora'");
}

void test_hito_getters() {
  Hito h(90, "Tres meses");
  ASSERT(h.getDiasRequeridos() == 90, "getDiasRequeridos() debe devolver 90");
  ASSERT(h.getDescripcion() == "Tres meses", "getDescripcion() debe devolver la descripcion dada");
}

void test_racha_incrementar_primera_vez() {
  Racha r;
  ASSERT(r.getDiasConsecutivos() == 0, "una racha nueva empieza en 0 dias");
  ASSERT(!r.estaActiva(), "una racha nueva no esta activa");

  r.incrementar();
  ASSERT(r.getDiasConsecutivos() == 1, "incrementar() una vez debe dejar 1 dia");
  ASSERT(r.estaActiva(), "incrementar() debe activar la racha");
  ASSERT(r.getFechaInicio() != 0, "incrementar() en dia 1 debe fijar fechaInicio");
}

void test_racha_incrementar_no_reinicia_fechaInicio() {
  Racha r;
  r.incrementar();
  time_t inicioDia1 = r.getFechaInicio();

  r.incrementar();
  ASSERT(r.getDiasConsecutivos() == 2, "incrementar() dos veces debe dejar 2 dias");
  ASSERT(r.getFechaInicio() == inicioDia1, "fechaInicio no debe cambiar tras el primer incremento");
}

void test_racha_reiniciar() {
  Racha r;
  r.incrementar();
  r.incrementar();
  r.reiniciar();

  ASSERT(r.getDiasConsecutivos() == 0, "reiniciar() debe dejar 0 dias");
  ASSERT(!r.estaActiva(), "reiniciar() debe dejar la racha inactiva");
  ASSERT(r.getFechaInicio() == 0, "reiniciar() debe limpiar fechaInicio");
}

void test_racha_verificarHito() {
  Racha r;
  for (int i = 0; i < 6; i++) r.incrementar();
  ASSERT(r.verificarHito() == nullptr, "no debe haber hito antes de los 7 dias");

  r.incrementar(); // dia 7
  Hito *hito = r.verificarHito();
  ASSERT(hito != nullptr, "debe haber un hito nuevo al llegar a 7 dias");
  if (hito != nullptr) {
    ASSERT(hito->getDiasRequeridos() == 7, "el hito de 7 dias debe tener diasRequeridos=7");
    ASSERT(hito->isCelebrado(), "verificarHito() debe marcar el hito como celebrado");
  }

  ASSERT(r.verificarHito() == nullptr, "no debe repetirse el mismo hito ya celebrado");

  for (int i = 0; i < 23; i++) r.incrementar(); // hasta dia 30
  Hito *hito30 = r.verificarHito();
  ASSERT(hito30 != nullptr && hito30->getDiasRequeridos() == 30, "debe celebrarse el hito de 30 dias");
}

void test_racha_reiniciar_resetea_hitos() {
  Racha r;
  for (int i = 0; i < 7; i++) r.incrementar(); // dia 7, celebra el hito
  ASSERT(r.verificarHito() != nullptr, "debe celebrarse el hito de 7 dias en la primera racha");

  r.reiniciar(); // se perdio la racha

  for (int i = 0; i < 7; i++) r.incrementar(); // segunda racha, llega a dia 7 otra vez
  Hito *hito = r.verificarHito();
  ASSERT(hito != nullptr, "una racha nueva que vuelve a llegar a 7 dias debe volver a celebrar el hito");
  ASSERT(hito->isCelebrado(), "el hito recien celebrado de nuevo debe quedar marcado");
}

void test_racha_restaurar() {
  Racha r;
  r.restaurar(15, 1000, 2000, true);
  ASSERT(r.getDiasConsecutivos() == 15, "restaurar() debe fijar diasConsecutivos");
  ASSERT(r.getFechaInicio() == 1000, "restaurar() debe fijar fechaInicio");
  ASSERT(r.getFechaUltimaInteraccion() == 2000, "restaurar() debe fijar fechaUltimaInteraccion");
  ASSERT(r.estaActiva(), "restaurar() debe fijar activa");
}

int main() {
  test_hito_estaAlcanzado();
  test_hito_marcarCelebrado();
  test_hito_restaurar();
  test_hito_getters();
  test_racha_incrementar_primera_vez();
  test_racha_incrementar_no_reinicia_fechaInicio();
  test_racha_reiniciar();
  test_racha_reiniciar_resetea_hitos();
  test_racha_verificarHito();
  test_racha_restaurar();

  printf("\n%d/%d pruebas pasaron\n", pruebasTotal - pruebasFallidas, pruebasTotal);
  return pruebasFallidas == 0 ? 0 : 1;
}
