#include "ComunicacionApp.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

void ComunicacionApp::init() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[ComunicacionApp] conectando a WiFi: ");
  Serial.println(WIFI_SSID);
}

bool ComunicacionApp::estaConectado() {
  return WiFi.status() == WL_CONNECTED;
}

bool ComunicacionApp::obtenerHora(uint8_t &horas, uint8_t &minutos) {
  if (!estaConectado()) {
    return false;
  }

  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/config/hora");
  http.setTimeout(3000);
  int codigo = http.GET();

  bool ok = false;
  if (codigo == 200) {
    StaticJsonDocument<128> doc;
    if (deserializeJson(doc, http.getString()) == DeserializationError::Ok && doc["ok"] == true) {
      horas = doc["horas"];
      minutos = doc["minutos"];
      ok = true;
    }
  } else {
    Serial.print("[ComunicacionApp] GET /config/hora fallo, codigo ");
    Serial.println(codigo);
  }

  http.end();
  return ok;
}

bool ComunicacionApp::reportarEstado(int dias, const char *estadoVisual, const char *evento) {
  if (!estaConectado()) {
    return false;
  }

  StaticJsonDocument<128> cuerpo;
  cuerpo["days"] = dias;
  cuerpo["totem_state"] = estadoVisual;
  cuerpo["event"] = evento;
  String payload;
  serializeJson(cuerpo, payload);

  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/device/sync");
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(3000);
  int codigo = http.POST(payload);

  if (codigo != 200) {
    Serial.print("[ComunicacionApp] POST /device/sync fallo, codigo ");
    Serial.println(codigo);
  }

  http.end();
  return codigo == 200;
}

bool ComunicacionApp::hayBorradoPendiente() {
  if (!estaConectado()) {
    return false;
  }

  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/device/wipe");
  http.setTimeout(3000);
  int codigo = http.GET();

  bool pendiente = false;
  if (codigo == 200) {
    StaticJsonDocument<64> doc;
    if (deserializeJson(doc, http.getString()) == DeserializationError::Ok) {
      pendiente = doc["pending"] | false;
    }
  }

  http.end();
  return pendiente;
}

void ComunicacionApp::confirmarBorrado() {
  if (!estaConectado()) {
    return;
  }

  HTTPClient http;
  http.begin(String(BACKEND_URL) + "/device/wipe/ack");
  http.setTimeout(3000);
  http.POST("");
  http.end();
}
