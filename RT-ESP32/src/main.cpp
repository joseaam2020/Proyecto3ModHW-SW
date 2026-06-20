/*
 Sobre la arquitectura pensada:
  El ESP32 actúa como CLIENTE HTTP del backend Flask.
  NO es un servidor que recibe programas; es el hardware
  físico del tótem que detecta sensores y controla luces.

  FLUJO:

  1. Conectar a WiFi
  2. Cada 2s: lee sensor de proximidad (IR / ultrasonido)
     Si detecta persona cerca: POST /proximity {active:true}
     Si se aleja:              POST /proximity {active:false}
     El backend cambia totem_state de "idle" a "active"
     El ESP32 sube la intensidad de los LEDs

  3. Continuamente: lee sensor táctil capacitivo (touch)
     Si se detecta contacto:  POST /touch
     El backend marca pending_checkin = true
     La app web (o el propio ESP32) ejecuta el check-in

  4. Cada 10s: GET /status
     Recibe: days, totem_state, today_message, pending_checkin
     Si pending_checkin es true y no se ha hecho hoy:
     POST /checkin → registra el día
     Actualiza LEDs y vibración según totem_state

  5. Sirve GET /health localmente para diagnóstico

  ENDPOINTS QUE CONSUME DEL BACKEND:
  ─────────────────────────────────────────────────────────
  POST /touch         avisa contacto del sensor táctil
  POST /proximity     avisa presencia / ausencia
  POST /checkin       registra un día de continuidad
  GET  /status        obtiene estado global (días, estado)
  GET  /today_message mensaje motivacional del día

  HARDWARE (ajusta los pines a tu circuito):
  ─────────────────────────────────────────────────────────
  GPIO 4  (T0)    Sensor táctil capacitivo (toca la parte superior)
  GPIO 15         Sensor de proximidad (digital: HIGH=detectado)
  GPIO 16         LED RGB - canal Rojo
  GPIO 17         LED RGB - canal Verde
  GPIO 18         LED RGB - canal Azul
  GPIO 19        Motor vibrador
  GPIO 2         LED integrado (indicador WiFi)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

// ═══════════════════════════════════════════════════════
// cambiar esto
// ═══════════════════════════════════════════════════════
const char* ssid       = "PuntaPaintillaPlus";
const char* password   = "1pasl24pkdsl";
const char* hostname   = "esp32-dev";

//ip
String BACKEND_URL     = "http://192.168.1.100:5000";

// ═══════════════════════════════════════════════════════
// PINES DE HARDWARE (CAMBIAR ESTO)
// ═══════════════════════════════════════════════════════
#define PIN_TOUCH       4     // T0 — sensor táctil capacitivo
#define PIN_PROXIMITY   15    // sensor de proximidad (digital)
#define PIN_LED_R       16    // LED RGB rojo
#define PIN_LED_G       17    // LED RGB verde
#define PIN_LED_B       18    // LED RGB azul
#define PIN_VIBRATOR    19    // motor vibrador
#define PIN_ONBOARD_LED 2     // LED integrado en la placa

// Umbral del sensor táctil (menor = más sensible)
#define TOUCH_THRESHOLD 40

// ═══════════════════════════════════════════════════════
// CANALES PWM 
// ═══════════════════════════════════════════════════════
#define CH_R  0
#define CH_G  1
#define CH_B  2
#define CH_VIB 3
#define PWM_FREQ  5000
#define PWM_RES   8     // 0-255

// ═══════════════════════════════════════════════════════
// ESTADOS DEL TÓTEM — colores y vibración
// ═══════════════════════════════════════════════════════
struct TotemVisual {
  uint8_t r, g, b;        // color LED
  uint8_t vibIntensity;   // 0-255
  uint16_t vibDuration;   // ms por pulso
  uint16_t vibPause;      // ms entre pulsos
};

// Cada estado tiene su patrón visual y háptico
const TotemVisual VISUAL_IDLE        = {  10,  15,  30,   0,    0,    0 };  // azul tenue
const TotemVisual VISUAL_ACTIVE      = { 255, 140,  30,   0,    0,    0 };  // ámbar cálido
const TotemVisual VISUAL_CONTACT     = { 255,  60,   0, 180,  200,  300 };  // naranja + vibra
const TotemVisual VISUAL_RISK        = { 245, 158,  11, 100,  100, 2000 };  // amarillo + pulso lento
const TotemVisual VISUAL_CELEBRATION = { 168,  85, 247, 220,  150,  200 };  // púrpura + vibra burst

// ═══════════════════════════════════════════════════════
// ESTADO GLOBAL
// ═══════════════════════════════════════════════════════
String totemState        = "idle";
int    currentDays       = 0;
String todayMessage      = "";
bool   proximityActive   = false;
bool   prevProximity     = false;
bool   touchDetected     = false;
bool   checkedInToday    = false;
unsigned long lastStatusPoll  = 0;
unsigned long lastProxCheck   = 0;
unsigned long lastVibToggle   = 0;
bool   vibOn                  = false;

// Servidor local para /health
WebServer localServer(80);

// ═══════════════════════════════════════════════════════
// FUNCIONES HTTP — comunicación con Flask
// ═══════════════════════════════════════════════════════

/**
 * POST genérico al backend. Retorna el JSON de respuesta.
 */
String httpPost(String endpoint, String jsonBody) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(BACKEND_URL + endpoint);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000);

  int code = http.POST(jsonBody);
  String response = "";
  if (code > 0) {
    response = http.getString();
    Serial.printf("[POST %s] %d\n", endpoint.c_str(), code);
  } else {
    Serial.printf("[POST %s] ERROR: %s\n", endpoint.c_str(), http.errorToString(code).c_str());
  }
  http.end();
  return response;
}

/**
 * GET genérico al backend. Retorna el JSON de respuesta.
 */
String httpGet(String endpoint) {
  if (WiFi.status() != WL_CONNECTED) return "";

  HTTPClient http;
  http.begin(BACKEND_URL + endpoint);
  http.setTimeout(5000);

  int code = http.GET();
  String response = "";
  if (code > 0) {
    response = http.getString();
    Serial.printf("[GET  %s] %d\n", endpoint.c_str(), code);
  } else {
    Serial.printf("[GET  %s] ERROR: %s\n", endpoint.c_str(), http.errorToString(code).c_str());
  }
  http.end();
  return response;
}


/**
 * Llama a POST /proximity cuando el sensor cambia de estado.
 * El backend cambia totem_state: idle ↔ active
 */
void notifyProximity(bool active) {
  String body = active ? "{\"active\":true}" : "{\"active\":false}";
  String resp = httpPost("/proximity", body);

  // Parsear respuesta para obtener el nuevo estado
  StaticJsonDocument<256> doc;
  if (!deserializeJson(doc, resp)) {
    totemState = doc["totem_state"] | totemState.c_str();
  }
  Serial.printf("Proximidad: %s → estado: %s\n",
    active ? "DETECTADO" : "LIBRE", totemState.c_str());
}

/**
 * Llama a POST /touch cuando el sensor táctil detecta contacto.
 * El backend marca pending_checkin = true.
 */
void notifyTouch() {
  String resp = httpPost("/touch", "{}");

  StaticJsonDocument<256> doc;
  if (!deserializeJson(doc, resp)) {
    totemState = doc["totem_state"] | "contact";
  }
  Serial.println("TOQUE detectado → estado: contact");
}

/**
 * Llama a POST /checkin para registrar el día.
 * El backend suma un día, detecta hitos, devuelve el nuevo estado.
 */
void doCheckin() {
  String resp = httpPost("/checkin", "{}");

  StaticJsonDocument<512> doc;
  if (!deserializeJson(doc, resp)) {
    bool ok = doc["ok"] | false;
    if (ok) {
      currentDays   = doc["days"] | currentDays;
      totemState    = doc["totem_state"] | "active";
      checkedInToday = true;
      bool milestone = doc["milestone"] | false;

      Serial.printf("CHECK-IN OK → día %d%s\n", currentDays,
        milestone ? " ★ HITO" : "");

      // Vibración de confirmación: 2 pulsos cortos
      vibratePattern(2, 150, 100);

      if (milestone) {
        totemState = "celebration";
        // Vibración de celebración: 5 pulsos rápidos
        vibratePattern(5, 100, 80);
      }
    } else {
      Serial.printf("CHECK-IN rechazado: %s\n", (const char*)(doc["error"] | "ya registrado"));
    }
  }
}

/**
 * GET /status — sincroniza el estado global cada N segundos.
 * Si pending_checkin es true, ejecuta el check-in automático.
 */
void pollStatus() {
  String resp = httpGet("/status");
  if (resp.length() == 0) return;

  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, resp)) return;

  currentDays    = doc["days"]          | currentDays;
  totemState     = doc["totem_state"]   | totemState.c_str();
  todayMessage   = doc["today_message"] | "";
  bool pending   = doc["pending_checkin"] | false;

  // Si el backend tiene un check-in pendiente (activado por /touch)
  // y aún no hemos hecho check-in hoy → ejecutar
  if (pending && !checkedInToday) {
    Serial.println("Check-in pendiente detectado → ejecutando...");
    doCheckin();
  }

  Serial.printf("STATUS → días=%d, estado=%s\n", currentDays, totemState.c_str());
}

/*para cosas de hardware:*/

/**
 * Establece el color del LED RGB usando PWM.
 */
void setLED(uint8_t r, uint8_t g, uint8_t b) {
  ledcWrite(CH_R, r);
  ledcWrite(CH_G, g);
  ledcWrite(CH_B, b);
}

/**
 * Efecto de respiración: sube y baja la intensidad.
 * Se llama en cada iteración del loop, usa millis() para no bloquear.
 */
void breatheLED(uint8_t r, uint8_t g, uint8_t b) {
  float phase = (sin(millis() / 1500.0) + 1.0) / 2.0;  // 0.0 - 1.0
  setLED((uint8_t)(r * phase), (uint8_t)(g * phase), (uint8_t)(b * phase));
}

/**
 * Patrón de vibración: N pulsos con duración y pausa (bloqueante).
 * Usada para feedback puntual (check-in confirmado, hito).
 */
void vibratePattern(int pulses, int onMs, int offMs) {
  for (int i = 0; i < pulses; i++) {
    ledcWrite(CH_VIB, 200);
    delay(onMs);
    ledcWrite(CH_VIB, 0);
    if (i < pulses - 1) delay(offMs);
  }
}

/**
 * Vibración no-bloqueante según el estado actual.
 * Se llama en cada iteración del loop.
 */
void updateVibration(const TotemVisual& v) {
  if (v.vibIntensity == 0) {
    ledcWrite(CH_VIB, 0);
    return;
  }
  unsigned long now = millis();
  if (vibOn) {
    if (now - lastVibToggle >= v.vibDuration) {
      ledcWrite(CH_VIB, 0);
      vibOn = false;
      lastVibToggle = now;
    }
  } else {
    if (now - lastVibToggle >= v.vibPause) {
      ledcWrite(CH_VIB, v.vibIntensity);
      vibOn = true;
      lastVibToggle = now;
    }
  }
}

/**
 * Actualiza LEDs y vibración según totemState.
 * Se llama en cada iteración del loop.
 */
void updateOutputs() {
  TotemVisual v;

  if      (totemState == "idle")        v = VISUAL_IDLE;
  else if (totemState == "active")      v = VISUAL_ACTIVE;
  else if (totemState == "contact")     v = VISUAL_CONTACT;
  else if (totemState == "risk")        v = VISUAL_RISK;
  else if (totemState == "celebration") v = VISUAL_CELEBRATION;
  else                                  v = VISUAL_IDLE;

  // LEDs: respiración para estados pasivos, fijo para contacto/celebración
  if (totemState == "idle" || totemState == "risk") {
    breatheLED(v.r, v.g, v.b);
  } else {
    setLED(v.r, v.g, v.b);
  }

  // Vibración no-bloqueante
  updateVibration(v);
}

//codigo para sensores
/**
 * Lee el sensor táctil capacitivo (T0 = GPIO4).
 * Retorna true si el valor está por debajo del umbral.
 */
bool readTouch() {
  int val = touchRead(PIN_TOUCH);
  return val < TOUCH_THRESHOLD;
}

/**
 * Lee el sensor de proximidad (digital).
 * Ajusta la lógica según tu sensor (HIGH o LOW = detectado).
 */
bool readProximity() {
  return digitalRead(PIN_PROXIMITY) == HIGH;
}

//endpoint /health para diagnostico del totem 

void handleLocalHealth() {
  localServer.sendHeader("Access-Control-Allow-Origin", "*");

  StaticJsonDocument<512> doc;
  doc["ok"]             = true;
  doc["device"]         = "EMBER Totem ESP32";
  doc["ip"]             = WiFi.localIP().toString();
  doc["rssi"]           = WiFi.RSSI();
  doc["backend"]        = BACKEND_URL;
  doc["totem_state"]    = totemState;
  doc["days"]           = currentDays;
  doc["proximity"]      = proximityActive;
  doc["checked_today"]  = checkedInToday;
  doc["uptime_sec"]     = millis() / 1000;
  doc["today_message"]  = todayMessage;

  String out;
  serializeJson(doc, out);
  localServer.send(200, "application/json", out);
}

//SETUPPPPPP
void setup() {
  Serial.begin(115200);
  Serial.println("\n══════════════════════════════");
  Serial.println("  EMBER Totem — Iniciando...");
  Serial.println("══════════════════════════════");

  // — Configurar pines —
  pinMode(PIN_ONBOARD_LED, OUTPUT);
  pinMode(PIN_PROXIMITY, INPUT);

  // — Configurar canales PWM —
  ledcSetup(CH_R,   PWM_FREQ, PWM_RES);
  ledcSetup(CH_G,   PWM_FREQ, PWM_RES);
  ledcSetup(CH_B,   PWM_FREQ, PWM_RES);
  ledcSetup(CH_VIB, PWM_FREQ, PWM_RES);

  ledcAttachPin(PIN_LED_R,    CH_R);
  ledcAttachPin(PIN_LED_G,    CH_G);
  ledcAttachPin(PIN_LED_B,    CH_B);
  ledcAttachPin(PIN_VIBRATOR, CH_VIB);

  // — Conexión WiFi —
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
  Serial.printf("Conectando a %s", ssid);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(PIN_ONBOARD_LED, HIGH);
    delay(250);
    digitalWrite(PIN_ONBOARD_LED, LOW);
    delay(250);
    Serial.print(".");
  }
  digitalWrite(PIN_ONBOARD_LED, HIGH);
  Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());

  // — mDNS —
  if (MDNS.begin(hostname)) {
    Serial.printf("mDNS: http://%s.local/\n", hostname);
  }

  // — Servidor local /health —
  localServer.on("/health", HTTP_GET, handleLocalHealth);
  localServer.begin();
  Serial.println("Servidor local iniciado en :80");

  // — Estado LED inicial: idle —
  setLED(VISUAL_IDLE.r, VISUAL_IDLE.g, VISUAL_IDLE.b);

  // — Primera sincronización —
  Serial.printf("Backend: %s\n", BACKEND_URL.c_str());
  pollStatus();

  Serial.println("══════════════════════════════");
  Serial.println("  EMBER listo. Esperando...");
  Serial.println("══════════════════════════════\n");
}

///MAINLOOOOOOP************************************
void loop() {
  unsigned long now = millis();

  // — Atender servidor local —
  localServer.handleClient();

  // — SENSOR DE PROXIMIDAD: cada 2 segundos —
  if (now - lastProxCheck >= 2000) {
    lastProxCheck = now;
    proximityActive = readProximity();

    // Solo notificar al backend cuando CAMBIA el estado
    if (proximityActive != prevProximity) {
      notifyProximity(proximityActive);
      prevProximity = proximityActive;
    }
  }

  // — SENSOR TÁCTIL: lectura continua con debounce —
  bool touching = readTouch();
  if (touching && !touchDetected) {
    touchDetected = true;
    Serial.println(">>> Toque detectado!");
    notifyTouch();

    // Si aún no ha hecho check-in hoy, hacerlo directamente
    if (!checkedInToday) {
      delay(500);  // breve pausa para feedback visual
      doCheckin();
    }
  }
  if (!touching) {
    touchDetected = false;  // reset cuando suelta
  }

  // — POLL STATUS: cada 10 segundos —
  if (now - lastStatusPoll >= 10000) {
    lastStatusPoll = now;
    pollStatus();
  }

  // — ACTUALIZAR SALIDAS (LEDs + vibración) —
  updateOutputs();

  // — Reset diario a medianoche (simple) —
  // Cuando el backend devuelva days=0 tras un reset,
  // checkedInToday se limpia en pollStatus si pending es false y days es 0
  // Para el reset por cambio de día, basta con que el backend
  // rechace duplicados y el ESP32 re-sincronice.

  delay(20);  // ~50 Hz loop
}