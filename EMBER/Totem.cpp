#include "Totem.h"
#include "Config.h"

Totem::Totem(const String &id)
  : id(id), estadoActual(REPOSO), conectado(false), ultimaInteraccion(0),
    umbralInactividad(UMBRAL_INACTIVIDAD_MS),
    sensorProximidad(TRIG_PIN, ECHO_PIN, UMBRAL_PROXIMIDAD_CM),
    sensorTactil(TOUCH_PIN, SENSIBILIDAD_TACTIL),
    led(LED_R, LED_G, LED_B),
    motor(MOTOR_PIN),
    tiempoEntradaEstado(0), ultimoRefrescoReloj(0),
    ultimaLecturaProximidadMs(0),
    inicioDiaMs(0), yaInteractuoHoy(false), limiteRiesgoMs(0),
    horaBaseHoras(0), horaBaseMinutos(0), horaBaseMillis(0) {
}

void Totem::encender() {
  sensorProximidad.init();
  sensorTactil.init();
  led.init();
  motor.init();
  pantalla.init();

  almacenamiento.init();
  almacenamiento.cargarRacha(racha);
  Serial.print("[Totem] racha cargada desde flash: ");
  Serial.print(racha.getDiasConsecutivos());
  Serial.println(" dias");

  // Arranca desde la última hora conocida (de un encendido anterior) en
  // vez de 00:00; se desactualiza con el tiempo apagado, pero se corrige
  // en cuanto el backend sincronice (Etapa 11, vía establecerHora()).
  almacenamiento.cargarHora(horaBaseHoras, horaBaseMinutos);
  horaBaseMillis = millis();

  estadoActual = REPOSO;
  tiempoEntradaEstado = millis();
  led.aplicarEstado(REPOSO);

  inicioDiaMs = millis();
  yaInteractuoHoy = false;

  refrescarReloj();
  pantalla.mostrarReloj();
  ultimoRefrescoReloj = millis();

  Serial.println("[Totem] encendido, estado = REPOSO");
}

void Totem::detectarProximidad() {
  // Solo aplica en Reposo: sube la intensidad de luz para indicar que
  // el tótem está listo para la interacción (CU-01, paso 2).
  //
  // Se limita a leer el sensor como máximo cada
  // INTERVALO_LECTURA_PROXIMIDAD_MS: disparar el HC-SR04 en cada loop()
  // sin pausa generaba interferencia entre pulsos consecutivos y hacía
  // que el LED parpadeara sin que hubiera nada enfrente.
  if (millis() - ultimaLecturaProximidadMs < INTERVALO_LECTURA_PROXIMIDAD_MS) {
    return;
  }
  ultimaLecturaProximidadMs = millis();

  float distancia = sensorProximidad.leer();

  // Sin eco (timeout, distancia <= 0) o más allá de
  // DISTANCIA_MAXIMA_VALIDA_CM: no hay nada relevante enfrente.
  if (distancia <= 0 || distancia > DISTANCIA_MAXIMA_VALIDA_CM) {
    led.setIntensidad(0.15f);
    return;
  }

  if (distancia <= UMBRAL_PROXIMIDAD_CM) {
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

  // CU-01 A2 (general): ya se registró la interacción de este "día"
  // (ver DURACION_DIA_MS) — no se puede volver a registrar hasta que
  // termine, sin importar cuánto tiempo pasó desde el último toque.
  if (yaInteractuoHoy) {
    Serial.println("[Totem] ya se registro la interaccion de este dia, toque ignorado");
    return;
  }

  ultimaInteraccion = time(nullptr);
  sensorTactil.registrarContacto();

  racha.incrementar();
  yaInteractuoHoy = true;
  almacenamiento.guardarRacha(racha);
  pantalla.mostrarProgreso(racha);
  Serial.print("[Totem] contacto registrado, racha = ");
  Serial.print(racha.getDiasConsecutivos());
  Serial.println(" dias");

  cambiarEstado(INGRESO_DIARIO);
  led.aplicarEstado(INGRESO_DIARIO);
  motor.aplicarEstado(INGRESO_DIARIO);
  registrarEventoPendiente(EVT_INTERACCION);

  Hito *hito = racha.verificarHito();
  if (hito != nullptr) {
    almacenamiento.guardarRacha(racha); // persiste el hito recién celebrado
    pantalla.mostrarHito(*hito);
    cambiarEstado(CELEBRACION);
    registrarEventoPendiente(EVT_HITO);
  }

  sincronizarConApp();
}

// Solo para diagnóstico por Serial (Etapa 9: las transiciones de Totem
// se validan manualmente en hardware, no con tests automatizados).
static const char* nombreEstado(SystemState e) {
  switch (e) {
    case REPOSO: return "REPOSO";
    case INGRESO_DIARIO: return "INGRESO_DIARIO";
    case CELEBRACION: return "CELEBRACION";
    case RIESGO_INACTIVIDAD: return "RIESGO_INACTIVIDAD";
    case REINICIO_RECUPERACION: return "REINICIO_RECUPERACION";
  }
  return "DESCONOCIDO";
}

void Totem::cambiarEstado(SystemState e) {
  estadoActual = e;
  tiempoEntradaEstado = millis();
  Serial.print("[Totem] estado -> ");
  Serial.println(nombreEstado(e));
}

void Totem::sincronizarConApp() {
  // TODO (Etapa 11): exponer estado/racha/hitos vía Wi-Fi/HTTP, llamar
  // establecerHora() con la hora que devuelva el backend, y vaciar la
  // cola de eventos pendientes. Por ahora solo se deja el punto de llamada.
  Serial.println("[Totem] sincronizarConApp() pendiente (Etapa 11)");
}

void Totem::verificarInactividad() {
  unsigned long ahora = millis();

  // Horario fijo de "día": pase lo que pase con los estados, si termina
  // el día se reabre la posibilidad de registrar la siguiente interacción.
  if (ahora - inicioDiaMs >= DURACION_DIA_MS) {
    inicioDiaMs += DURACION_DIA_MS;
    yaInteractuoHoy = false;
    Serial.println("[Totem] nuevo dia, toque disponible de nuevo");

    // UMBRAL_INACTIVIDAD_MS es un tiempo "dentro de un día": al empezar
    // uno nuevo se reinicia desde cero, sin arrastrar lo ya transcurrido
    // del día anterior.
    if (estadoActual == REPOSO) {
      tiempoEntradaEstado = ahora;
    }
  }

  switch (estadoActual) {
    case REPOSO:
      // No se marca Riesgo si ya se cumplió la interacción de hoy —
      // eso sería avisar de un peligro que ya no existe.
      if (!yaInteractuoHoy && (ahora - tiempoEntradaEstado >= umbralInactividad)) {
        cambiarEstado(RIESGO_INACTIVIDAD);
        led.aplicarEstado(RIESGO_INACTIVIDAD);
        // El plazo de Riesgo no es una duración fija propia: es lo que
        // queda del día actual. Si se entra tarde en el día, queda menos
        // margen; si se entra temprano, queda más. Se fija una sola vez
        // aquí y no se reajusta con el horario de "día" (ver
        // Totem::limiteRiesgoMs).
        limiteRiesgoMs = inicioDiaMs + DURACION_DIA_MS;
      }
      break;

    case INGRESO_DIARIO:
      if (ahora - tiempoEntradaEstado >= DURACION_CONFIRMACION_MS) {
        cambiarEstado(REPOSO);
        led.aplicarEstado(REPOSO);
        // Sin esto, la pantalla se quedaba en "Racha: N dias" hasta el
        // siguiente refresco periódico del reloj (hasta 1 min después).
        refrescarReloj();
        pantalla.mostrarReloj();
        ultimoRefrescoReloj = millis();
      }
      break;

    case CELEBRACION:
      if (ahora - tiempoEntradaEstado >= DURACION_CELEBRACION_MS) {
        cambiarEstado(REPOSO);
        led.aplicarEstado(REPOSO);
        // Sin esto, la pantalla se quedaba en "Hito: N dias!" hasta el
        // siguiente refresco periódico del reloj (hasta 1 min después).
        refrescarReloj();
        pantalla.mostrarReloj();
        ultimoRefrescoReloj = millis();
      }
      break;

    case RIESGO_INACTIVIDAD:
      if (ahora >= limiteRiesgoMs) {
        racha.reiniciar();
        almacenamiento.guardarRacha(racha);
        cambiarEstado(REINICIO_RECUPERACION);
        led.aplicarEstado(REINICIO_RECUPERACION);
        refrescarReloj();
        pantalla.mostrarReinicio();
        ultimoRefrescoReloj = millis();
        registrarEventoPendiente(EVT_REINICIO);
        sincronizarConApp();
      }
      break;

    case REINICIO_RECUPERACION:
      // Sin timeout: se queda aquí indefinidamente hasta que el usuario
      // toque el sensor (ver Totem::actualizar()). No pasa a Riesgo ni
      // vuelve solo a Reposo.
      break;
  }
}

void Totem::establecerHora(uint8_t horas, uint8_t minutos) {
  horaBaseHoras = horas;
  horaBaseMinutos = minutos;
  horaBaseMillis = millis();
  almacenamiento.guardarHora(horas, minutos);

  // Refleja la corrección de inmediato, sin esperar al próximo tick de
  // INTERVALO_RELOJ_MS.
  refrescarReloj();
  pantalla.mostrarReloj();
  ultimoRefrescoReloj = millis();
}

void Totem::refrescarReloj() {
  // Hora base + minutos transcurridos desde que se fijó esa base.
  unsigned long minutosTranscurridos = (millis() - horaBaseMillis) / 60000UL;
  unsigned long totalMinutos = (unsigned long)horaBaseHoras * 60UL + horaBaseMinutos + minutosTranscurridos;

  uint8_t horas = (totalMinutos / 60UL) % 24;
  uint8_t minutos = totalMinutos % 60UL;
  pantalla.actualizarHora(horas, minutos);
}

void Totem::registrarEventoPendiente(TipoEventoSync tipo) {
  if (!conectado) {
    almacenamiento.encolarEvento(tipo);
  }
  // TODO (Etapa 11): si conectado, enviar de inmediato por HTTP en vez de
  // encolar (o encolar igual y dejar que sincronizarConApp() lo vacíe).
}

void Totem::actualizar() {
  motor.actualizar(); // apaga el motor no bloqueante si corresponde

  switch (estadoActual) {
    case REPOSO:
      detectarProximidad();
      if (sensorTactil.detectarToqueNuevo()) {
        registrarContacto();
      } else if (millis() - ultimoRefrescoReloj >= INTERVALO_RELOJ_MS) {
        refrescarReloj();
        pantalla.mostrarReloj();
        ultimoRefrescoReloj = millis();
      }
      break;

    case RIESGO_INACTIVIDAD:
      motor.aplicarEstado(RIESGO_INACTIVIDAD); // recordatorio periódico (Etapa 3)
      if (sensorTactil.detectarToqueNuevo()) {
        registrarContacto();
      }
      break;

    case CELEBRACION:
      led.aplicarEstado(CELEBRACION); // anima cada frame
      break;

    case REINICIO_RECUPERACION:
      // Funciona igual que Reposo (proximidad, toque, reloj), solo que
      // con luz azul y el mensaje "Racha reiniciada" en pantalla. No hay
      // timeout: se queda aquí hasta que el usuario toque.
      detectarProximidad();
      if (sensorTactil.detectarToqueNuevo()) {
        registrarContacto();
      } else if (millis() - ultimoRefrescoReloj >= INTERVALO_RELOJ_MS) {
        refrescarReloj();
        pantalla.mostrarReloj();
        ultimoRefrescoReloj = millis();
      }
      break;

    case INGRESO_DIARIO:
      // Sin lectura activa de sensores: solo espera el timeout
      // correspondiente en verificarInactividad().
      break;
  }

  verificarInactividad();
}

SystemState Totem::getEstadoActual() const {
  return estadoActual;
}
