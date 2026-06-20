#ifndef CONFIG_H
#define CONFIG_H

// ── Pines de hardware (igual que test/test.ino) ─────────────────────
#define TRIG_PIN  5
#define ECHO_PIN  18
#define TOUCH_PIN 4
#define LED_R     22
#define LED_G     23
#define LED_B     19
#define MOTOR_PIN 21
// e-Paper: CLK=D13, DIN=D14, CS=D15, DC=D27, RST=D26, BUSY=D25
// (pines fijados por la librería esp32-waveshare-epd vía DEV_Config.h)

// ── Modo demo ────────────────────────────────────────────────────────
// Con MODO_DEMO definido, los tiempos de espera (pensados para 24h
// reales) se comprimen a segundos, para poder ver Riesgo/Reinicio en una
// demo corta sin esperar un día real. Comentar esta línea para el
// comportamiento real.
//
// IMPORTANTE: ya no es posible registrar más de una interacción por
// "día" (ver DURACION_DIA_MS) — alcanzar los hitos de 30/90 días en este
// modo demo toma ~30 min / ~1.5 horas reales (con DURACION_DIA_MS = 1 min),
// no es práctico para una demo en vivo. Si se necesita demostrar esos
// hitos rápido, hay que decidir un mecanismo aparte (ej. acortar
// HITO_DIAS_2/3 solo para demo).
#define MODO_DEMO

// ── Tiempos del ciclo de vida de la racha ───────────────────────────
#ifdef MODO_DEMO
  // Periodo dentro del cual se espera la interacción diaria antes de
  // pasar a Riesgo/Inactividad. El plazo de Riesgo en sí NO es un valor
  // aparte: es lo que queda del día (DURACION_DIA_MS - este valor) desde
  // que se entra a Riesgo — ver Totem::limiteRiesgoMs.
  #define UMBRAL_INACTIVIDAD_MS            (30UL * 1000UL)        // 30 s (real: 24h)

  // Duración fija de "un día": una vez registrada la interacción, no se
  // acepta otra hasta que termine este período (sin importar en qué
  // momento del día se tocó).
  #define DURACION_DIA_MS                  (1UL * 60UL * 1000UL)  // 1 min (real: 24h)

  // Recordatorio háptico mientras el tótem está en Riesgo/Inactividad.
  // Debe ser mayor que DURACION_RECORDATORIO_RIESGO_MS (más abajo) para
  // que se sienta como pulsos separados y no como vibración continua.
  #define INTERVALO_RECORDATORIO_RIESGO_MS (10UL * 1000UL)   // 10 s (real: 1h)

  // Intervalo de refresco visual del reloj. En real es fijo a 1 min (es
  // solo la hora del día, no tiene sentido comprimirlo); en demo se
  // acorta para poder verlo actualizarse dentro de los timers ya
  // comprimidos (Reposo dura solo 30s antes de pasar a Riesgo).
  #define INTERVALO_RELOJ_MS                (10UL * 1000UL)  // 10 s (real: 1 min)
#else
  // Mitad del día: deja las otras 12h como plazo de Riesgo antes de
  // perder la racha (ver Totem::limiteRiesgoMs).
  #define UMBRAL_INACTIVIDAD_MS            (12UL * 60UL * 60UL * 1000UL)  // 12 h
  #define DURACION_DIA_MS                  (24UL * 60UL * 60UL * 1000UL)  // 24 h
  #define INTERVALO_RECORDATORIO_RIESGO_MS (60UL * 60UL * 1000UL)         // 1 h
  #define INTERVALO_RELOJ_MS               (60UL * 1000UL)                // 1 min
#endif

// Duración de la secuencia transitoria de Celebración (hito alcanzado).
#define DURACION_CELEBRACION_MS (30UL * 1000UL)                // 30 s

// Vibración del recordatorio en Riesgo/Inactividad: más fuerte y más
// larga que antes (antes: intensidad 0.7, 3 s).
#define DURACION_RECORDATORIO_RIESGO_MS  (6UL * 1000UL)         // 6 s

// Animación del LED en Celebración: paso de arcoíris cada cuántos ms, y
// cuántos grados de hue avanza por paso. Con estos valores el ciclo
// completo de color toma ~21.6 s (360° / 1° por paso × 60 ms).
#define INTERVALO_PASO_ARCOIRIS_MS 60UL
#define INCREMENTO_HUE_ARCOIRIS    1.0f

// ── Hitos de continuidad (días consecutivos) ────────────────────────
#define HITO_DIAS_1 7
#define HITO_DIAS_2 30
#define HITO_DIAS_3 90

// ── Sensores ─────────────────────────────────────────────────────────
#define UMBRAL_PROXIMIDAD_CM    30.0f
#define SENSIBILIDAD_TACTIL     1.0f

// Más allá de esta distancia (o sin eco/timeout) se considera que no hay
// nada relevante enfrente: el LED se mantiene tenue sin importar ruido.
#define DISTANCIA_MAXIMA_VALIDA_CM     50.0f

// Mínimo tiempo entre lecturas del HC-SR04 en Reposo. Leerlo en cada
// loop() sin pausa generaba interferencia entre pulsos consecutivos y
// hacía que el LED parpadeara sin que hubiera nada enfrente.
#define INTERVALO_LECTURA_PROXIMIDAD_MS 150UL

// ── Estado Ingreso Diario ────────────────────────────────────────────
// Cuánto se mantiene la confirmación visual antes de volver a Reposo
// ("Termina periodo" en el diagrama de estados).
#define DURACION_CONFIRMACION_MS (20UL * 1000UL)               // 20 s

#endif
