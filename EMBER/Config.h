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

// ── Tiempos del ciclo de vida de la racha ───────────────────────────
// Periodo dentro del cual se espera la interacción diaria antes de
// pasar a Riesgo/Inactividad.
#define UMBRAL_INACTIVIDAD_MS   (24UL * 60UL * 60UL * 1000UL)  // 24 h

// Periodo de gracia en Riesgo/Inactividad antes de perder la racha.
#define PERIODO_RIESGO_MS       (24UL * 60UL * 60UL * 1000UL)  // 24 h

// Duración de la secuencia transitoria de Celebración.
#define DURACION_CELEBRACION_MS (5UL * 1000UL)                 // 5 s

// Tiempo en Reinicio/Recuperación antes de volver a Reposo (fin de día).
#define PERIODO_REINICIO_MS     (24UL * 60UL * 60UL * 1000UL)  // 24 h

// Recordatorio háptico mientras el tótem está en Riesgo/Inactividad:
// vibra DURACION_RECORDATORIO_RIESGO_MS cada INTERVALO_RECORDATORIO_RIESGO_MS.
#define INTERVALO_RECORDATORIO_RIESGO_MS (60UL * 60UL * 1000UL) // 1 h
#define DURACION_RECORDATORIO_RIESGO_MS  (3UL * 1000UL)         // 3 s

// ── Hitos de continuidad (días consecutivos) ────────────────────────
#define HITO_DIAS_1 7
#define HITO_DIAS_2 30
#define HITO_DIAS_3 90

// ── Pantalla E-Ink ───────────────────────────────────────────────────
// Intervalo de refresco parcial del reloj mientras el tótem está en Reposo.
#define INTERVALO_RELOJ_MS      (60UL * 1000UL)                // 1 min

// ── Sensores ─────────────────────────────────────────────────────────
#define UMBRAL_PROXIMIDAD_CM    30.0f
#define SENSIBILIDAD_TACTIL     1.0f

// ── Estado Ingreso Diario ────────────────────────────────────────────
// Cuánto se mantiene la confirmación visual antes de volver a Reposo
// ("Termina periodo" en el diagrama de estados).
#define DURACION_CONFIRMACION_MS (5UL * 1000UL)                // 5 s

#endif
