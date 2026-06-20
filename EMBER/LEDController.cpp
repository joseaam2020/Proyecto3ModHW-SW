#include "LEDController.h"
#include "Config.h"

LEDController::LEDController(uint8_t rPin, uint8_t gPin, uint8_t bPin)
  : rPin(rPin), gPin(gPin), bPin(bPin), intensidad(1.0f), patron("solido") {
  colorActual[0] = colorActual[1] = colorActual[2] = 0;
}

void LEDController::init() {
  ledcAttach(rPin, 5000, 8);
  ledcAttach(gPin, 5000, 8);
  ledcAttach(bPin, 5000, 8);
}

// LED común ánodo: el pin común va a 3V3, así que un canal "más encendido"
// corresponde a un duty más bajo (igual que en test/test.ino).
void LEDController::write(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(rPin, 255 - r);
  ledcWrite(gPin, 255 - g);
  ledcWrite(bPin, 255 - b);
}

void LEDController::setColor(uint8_t r, uint8_t g, uint8_t b) {
  colorActual[0] = r;
  colorActual[1] = g;
  colorActual[2] = b;
  write((uint8_t)(r * intensidad), (uint8_t)(g * intensidad), (uint8_t)(b * intensidad));
}

void LEDController::setIntensidad(float v) {
  intensidad = constrain(v, 0.0f, 1.0f);
  write((uint8_t)(colorActual[0] * intensidad),
        (uint8_t)(colorActual[1] * intensidad),
        (uint8_t)(colorActual[2] * intensidad));
}

void LEDController::aplicarEstado(SystemState estado) {
  switch (estado) {
    case REPOSO:
      patron = "ambiental";
      setIntensidad(0.15f);
      setColor(255, 180, 80); // blanco cálido tenue
      break;

    case INGRESO_DIARIO:
      patron = "confirmacion";
      setIntensidad(1.0f);
      setColor(60, 220, 90); // verde — continuidad confirmada
      break;

    case RIESGO_INACTIVIDAD:
      patron = "alerta";
      setIntensidad(0.8f);
      setColor(255, 0, 0); // rojo — advertencia
      break;

    case REINICIO_RECUPERACION:
      patron = "acompanamiento";
      setIntensidad(0.6f);
      setColor(90, 140, 255); // azul suave — recuperación, no castigo
      break;

    case CELEBRACION: {
      // Animación arcoíris lenta: el color solo avanza cada
      // INTERVALO_PASO_ARCOIRIS_MS, aunque esto se llame en cada
      // loop() (que corre mucho más rápido que eso).
      patron = "celebracion";
      static unsigned long ultimoPaso = 0;
      static float hue = 0.0f;

      if (millis() - ultimoPaso < INTERVALO_PASO_ARCOIRIS_MS) {
        break;
      }
      ultimoPaso = millis();
      hue = (hue >= 360.0f) ? 0.0f : hue + INCREMENTO_HUE_ARCOIRIS;

      int i = (int)(hue / 60.0f) % 6;
      float f = (hue / 60.0f) - (int)(hue / 60.0f);
      float r, g, b;
      switch (i) {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = 1 - f; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = 1 - f; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        default: r = 1; g = 0; b = 1 - f; break;
      }
      setIntensidad(1.0f);
      setColor((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
      break;
    }
  }
}
