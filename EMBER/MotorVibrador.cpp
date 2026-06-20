#include "MotorVibrador.h"
#include "Config.h"

MotorVibrador::MotorVibrador(uint8_t pin)
  : pin(pin), intensidad(1.0f), duracion(0), patron("ninguno"),
    activo(false), inicioMs(0), ultimoRecordatorioMs(0) {
}

void MotorVibrador::init() {
  ledcAttach(pin, 5000, 8);
  ledcWrite(pin, 0);
}

void MotorVibrador::setIntensidad(float v) {
  intensidad = constrain(v, 0.0f, 1.0f);
  if (activo) {
    ledcWrite(pin, (uint8_t)(255 * intensidad));
  }
}

void MotorVibrador::vibrar(unsigned long ms) {
  duracion = ms;
  inicioMs = millis();
  activo = true;
  ledcWrite(pin, (uint8_t)(255 * intensidad));
}

void MotorVibrador::actualizar() {
  if (activo && (millis() - inicioMs >= duracion)) {
    ledcWrite(pin, 0);
    activo = false;
  }
}

void MotorVibrador::aplicarEstado(SystemState estado) {
  switch (estado) {
    case INGRESO_DIARIO:
      // Vibración fuerte y continua durante toda la confirmación visual
      // (mismos DURACION_CONFIRMACION_MS que se muestra "Racha: N dias").
      patron = "confirmacion";
      setIntensidad(1.0f);
      vibrar(DURACION_CONFIRMACION_MS);
      break;

    case CELEBRACION:
      // Vibración más intensa para acompañar la secuencia de logro.
      patron = "celebracion";
      setIntensidad(1.0f);
      vibrar(400);
      break;

    case RIESGO_INACTIVIDAD:
      // Recordatorio háptico periódico mientras la racha está en riesgo.
      patron = "recordatorio";
      if (millis() - ultimoRecordatorioMs >= INTERVALO_RECORDATORIO_RIESGO_MS) {
        ultimoRecordatorioMs = millis();
        setIntensidad(1.0f);
        vibrar(DURACION_RECORDATORIO_RIESGO_MS);
      }
      break;

    case REPOSO:
    case REINICIO_RECUPERACION:
      // Sin vibración: estos estados solo comunican mediante luz
      // (y notificación a la app, fuera del alcance de esta clase).
      patron = "ninguno";
      break;
  }
}
