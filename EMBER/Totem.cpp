#include "Totem.h"
#include "Config.h"

Totem::Totem(const String &id)
  : id(id), estadoActual(REPOSO), conectado(false), ultimaInteraccion(0),
    umbralInactividad(UMBRAL_INACTIVIDAD_MS),
    sensorProximidad(TRIG_PIN, ECHO_PIN, UMBRAL_PROXIMIDAD_CM),
    sensorTactil(TOUCH_PIN, SENSIBILIDAD_TACTIL),
    led(LED_R, LED_G, LED_B),
    motor(MOTOR_PIN),
    tiempoEntradaEstado(0), ultimoRefrescoReloj(0) {
}

void Totem::encender() {
  sensorProximidad.init();
  sensorTactil.init();
  led.init();
  motor.init();
  pantalla.init();

  // TODO (Etapa 7): cargar racha/hitos/offset de hora desde flash aquí,
  // en vez de arrancar siempre desde cero.

  estadoActual = REPOSO;
  tiempoEntradaEstado = millis();
  led.aplicarEstado(REPOSO);

  refrescarRelojPlaceholder();
  pantalla.mostrarReloj();
  ultimoRefrescoReloj = millis();

  Serial.println("[Totem] encendido, estado = REPOSO");
}

void Totem::detectarProximidad() {
  // Solo aplica en Reposo: sube la intensidad de luz para indicar que
  // el tótem está listo para la interacción (CU-01, paso 2).
  if (sensorProximidad.detectarPresencia()) {
    led.setIntensidad(0.6f);
  } else {
    led.setIntensidad(0.15f);
  }
}

void Totem::registrarContacto() {
  // CU-01 A2: si ya estamos en un estado "post-confirmación" del mismo
  // ciclo, no se duplica el registro de racha.
  if (estadoActual == INGRESO_DIARIO || estadoActual == CELEBRACION) {
    return;
  }

  ultimaInteraccion = time(nullptr);
  sensorTactil.registrarContacto();

  racha.incrementar();
  pantalla.mostrarProgreso(racha);

  cambiarEstado(INGRESO_DIARIO);
  led.aplicarEstado(INGRESO_DIARIO);
  motor.aplicarEstado(INGRESO_DIARIO);

  Hito *hito = racha.verificarHito();
  if (hito != nullptr) {
    pantalla.mostrarHito(*hito);
    cambiarEstado(CELEBRACION);
  }

  sincronizarConApp();
}

void Totem::cambiarEstado(SystemState e) {
  estadoActual = e;
  tiempoEntradaEstado = millis();
}

void Totem::sincronizarConApp() {
  // TODO (Etapa 8): exponer estado/racha/hitos vía BLE y vaciar la cola
  // de eventos pendientes. Por ahora solo se deja el punto de llamada.
  Serial.println("[Totem] sincronizarConApp() pendiente (Etapa 8)");
}

void Totem::verificarInactividad() {
  unsigned long ahora = millis();

  switch (estadoActual) {
    case REPOSO:
      if (ahora - tiempoEntradaEstado >= umbralInactividad) {
        cambiarEstado(RIESGO_INACTIVIDAD);
        led.aplicarEstado(RIESGO_INACTIVIDAD);
      }
      break;

    case INGRESO_DIARIO:
      if (ahora - tiempoEntradaEstado >= DURACION_CONFIRMACION_MS) {
        cambiarEstado(REPOSO);
        led.aplicarEstado(REPOSO);
      }
      break;

    case CELEBRACION:
      if (ahora - tiempoEntradaEstado >= DURACION_CELEBRACION_MS) {
        cambiarEstado(INGRESO_DIARIO);
        led.aplicarEstado(INGRESO_DIARIO);
      }
      break;

    case RIESGO_INACTIVIDAD:
      if (ahora - tiempoEntradaEstado >= PERIODO_RIESGO_MS) {
        racha.reiniciar();
        cambiarEstado(REINICIO_RECUPERACION);
        led.aplicarEstado(REINICIO_RECUPERACION);
        sincronizarConApp();
      }
      break;

    case REINICIO_RECUPERACION:
      if (ahora - tiempoEntradaEstado >= PERIODO_REINICIO_MS) {
        cambiarEstado(REPOSO);
        led.aplicarEstado(REPOSO);
      }
      break;
  }
}

void Totem::refrescarRelojPlaceholder() {
  // Placeholder temporal: deriva una "hora" de millis() desde el boot.
  // Se reemplaza por la hora real persistida/sincronizada en Etapa 7/8.
  unsigned long segundosTotales = millis() / 1000UL;
  uint8_t horas = (segundosTotales / 3600UL) % 24;
  uint8_t minutos = (segundosTotales / 60UL) % 60;
  pantalla.actualizarHora(horas, minutos);
}

void Totem::actualizar() {
  motor.actualizar(); // apaga el motor no bloqueante si corresponde

  switch (estadoActual) {
    case REPOSO:
      detectarProximidad();
      if (sensorTactil.leer()) {
        registrarContacto();
      } else if (millis() - ultimoRefrescoReloj >= INTERVALO_RELOJ_MS) {
        refrescarRelojPlaceholder();
        pantalla.mostrarReloj();
        ultimoRefrescoReloj = millis();
      }
      break;

    case RIESGO_INACTIVIDAD:
      motor.aplicarEstado(RIESGO_INACTIVIDAD); // recordatorio periódico (Etapa 3)
      if (sensorTactil.leer()) {
        registrarContacto();
      }
      break;

    case CELEBRACION:
      led.aplicarEstado(CELEBRACION); // anima cada frame
      break;

    case INGRESO_DIARIO:
    case REINICIO_RECUPERACION:
      // Sin lectura activa de sensores: solo esperan el timeout
      // correspondiente en verificarInactividad().
      break;
  }

  verificarInactividad();
}

SystemState Totem::getEstadoActual() const {
  return estadoActual;
}
