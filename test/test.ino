/* EMBER — System Hardware Test
 * Tests all hardware components together:
 *   - e-Paper 4.2"  : displays Hello World on startup
 *   - HC-SR04       : reads distance, prints to Serial every 5s
 *   - Touch sensor  : reads state, prints to Serial every 5s
 *   - RGB LED       : continuous rainbow effect
 *   - Vibration motor (NMOS): fires for 1s every 5s
 *
 * Pin assignments:
 *   e-Paper : CLK=D13, DIN=D14, CS=D15, DC=D27, RST=D26, BUSY=D25
 *   HC-SR04 : TRIG=D5, ECHO=D18 (via 1kΩ+2kΩ voltage divider)
 *   Touch   : I/O=D4
 *   LED RGB : R=D22, G=D23, B=D19  (common anode — common pin to 3V3)
 *   Motor   : NMOS gate=D21
 */

#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"
#include <stdlib.h>

// ── Pin definitions ──────────────────────────────────────────────
#define TRIG_PIN  5
#define ECHO_PIN  18
#define TOUCH_PIN 4
#define LED_R     22
#define LED_G     23
#define LED_B     19
#define MOTOR_PIN 21

// ── Timing ───────────────────────────────────────────────────────
#define SENSOR_INTERVAL 5000UL  // ms between sensor reads
#define MOTOR_ON_TIME   1000UL  // ms motor stays on

unsigned long lastSensorTime = 0;
unsigned long motorStartTime = 0;
bool motorActive = false;
float hue = 0.0f;

// ── HC-SR04 distance reading ──────────────────────────────────────
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long d = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout (~5m max range)
  return (d == 0) ? -1.0f : d * 0.034f / 2.0f;
}

// ── HSV → RGB → LEDC write ───────────────────────────────────────
void setLedHSV(float h, float s, float v) {
  int i = (int)(h / 60.0f) % 6;
  float f = (h / 60.0f) - (int)(h / 60.0f);
  float p = v * (1.0f - s);
  float q = v * (1.0f - f * s);
  float t = v * (1.0f - (1.0f - f) * s);
  float r, g, b;
  switch (i) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default: r=v; g=p; b=q; break;
  }
  ledcWrite(LED_R, 255 - (uint8_t)(r * 255));
  ledcWrite(LED_G, 255 - (uint8_t)(g * 255));
  ledcWrite(LED_B, 255 - (uint8_t)(b * 255));
}

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Touch sensor
  pinMode(TOUCH_PIN, INPUT);

  // Motor
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // RGB LED — PWM via LEDC (5 kHz, 8-bit resolution → duty 0-255)
  ledcAttach(LED_R, 5000, 8);
  ledcAttach(LED_G, 5000, 8);
  ledcAttach(LED_B, 5000, 8);

  // e-Paper: draw Hello World and go to sleep
  DEV_Module_Init();

  // Three init+clear cycles: each re-initializes the controller and re-applies the full
  // waveform to all pixels. Multiple passes are needed to drive out ghost content left
  // by previous programs (single-pass fast-LUT clear is not always strong enough).
  for (int cp = 0; cp < 3; cp++) {
    EPD_4IN2_Init_Fast();
    EPD_4IN2_Clear();
    DEV_Delay_ms(500);
  }

  UWORD imgSize = ((EPD_4IN2_WIDTH % 8 == 0)
    ? (EPD_4IN2_WIDTH / 8)
    : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;

  UBYTE *img = (UBYTE *)malloc(imgSize);
  if (img == NULL) {
    Serial.println("[e-Paper] malloc failed — halting");
    while (1);
  }

  // rotation=270: logical(0,0) → physical(0,299), which for a 90°CCW-mounted display
  // (cable at bottom) maps to portrait(0,0) = top-left corner
  Paint_NewImage(img, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 270, WHITE);
  Paint_SelectImage(img);
  Paint_Clear(WHITE);
  Paint_DrawString_EN(10, 10, "EMBER",                &Font16, BLACK, WHITE);
  Paint_DrawString_EN(10, 35, "Hello World",           &Font16, BLACK, WHITE);
  Paint_DrawString_EN(10, 60, "System test running.",  &Font12, BLACK, WHITE);
  EPD_4IN2_Display(img);

  EPD_4IN2_Sleep();
  free(img);

  Serial.println("EMBER system test started.");
  Serial.println("Sensor reads + motor fire every 5s. Rainbow LED running.");
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Turn motor off after MOTOR_ON_TIME
  if (motorActive && (now - motorStartTime >= MOTOR_ON_TIME)) {
    digitalWrite(MOTOR_PIN, LOW);
    motorActive = false;
    Serial.println("[Motor]     OFF");
  }

  // Every 5s: read sensors and fire motor
  if (now - lastSensorTime >= SENSOR_INTERVAL) {
    lastSensorTime = now;

    // HC-SR04
    float dist = readDistanceCm();
    Serial.print("[Proximity] ");
    if (dist < 0) {
      Serial.println("no object detected");
    } else {
      Serial.print(dist, 1);
      Serial.println(" cm");
    }

    // Capacitive touch
    bool touched = digitalRead(TOUCH_PIN);
    Serial.print("[Touch]     ");
    Serial.println(touched ? "TOUCHED" : "not touched");

    // Motor
    digitalWrite(MOTOR_PIN, HIGH);
    motorActive    = true;
    motorStartTime = now;
    Serial.println("[Motor]     ON");
  }

  // Rainbow — full hue cycle in ~14 s (0.5 deg/20 ms × 720 steps)
  hue = (hue >= 360.0f) ? 0.0f : hue + 0.5f;
  setLedHSV(hue, 1.0f, 1.0f);
  delay(20);
}
