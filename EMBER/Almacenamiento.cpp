#include "Almacenamiento.h"

void Almacenamiento::init() {
  prefs.begin("ember", false);
}

void Almacenamiento::guardarRacha(Racha &racha) {
  prefs.putInt("r_dias", racha.getDiasConsecutivos());
  prefs.putULong("r_ini", (unsigned long)racha.getFechaInicio());
  prefs.putULong("r_ult", (unsigned long)racha.getFechaUltimaInteraccion());
  prefs.putBool("r_act", racha.estaActiva());

  for (int i = 0; i < 3; i++) {
    guardarHito(i, racha.getHito(i));
  }
}

void Almacenamiento::cargarRacha(Racha &racha) {
  int dias = prefs.getInt("r_dias", 0);
  time_t inicio = (time_t)prefs.getULong("r_ini", 0);
  time_t ultima = (time_t)prefs.getULong("r_ult", 0);
  bool activa = prefs.getBool("r_act", false);
  racha.restaurar(dias, inicio, ultima, activa);

  for (int i = 0; i < 3; i++) {
    bool celebrado;
    time_t fecha;
    cargarHito(i, celebrado, fecha);
    if (celebrado) {
      racha.getHito(i).restaurar(true, fecha);
    }
  }
}

void Almacenamiento::guardarHito(int indice, Hito &hito) {
  String keyCel = "h" + String(indice) + "_cel";
  String keyFecha = "h" + String(indice) + "_f";
  prefs.putBool(keyCel.c_str(), hito.isCelebrado());
  prefs.putULong(keyFecha.c_str(), (unsigned long)hito.getFechaAlcanzado());
}

void Almacenamiento::cargarHito(int indice, bool &celebrado, time_t &fechaAlcanzado) {
  String keyCel = "h" + String(indice) + "_cel";
  String keyFecha = "h" + String(indice) + "_f";
  celebrado = prefs.getBool(keyCel.c_str(), false);
  fechaAlcanzado = (time_t)prefs.getULong(keyFecha.c_str(), 0);
}

void Almacenamiento::guardarHora(uint8_t horas, uint8_t minutos) {
  prefs.putUChar("hr_h", horas);
  prefs.putUChar("hr_m", minutos);
}

void Almacenamiento::cargarHora(uint8_t &horas, uint8_t &minutos) {
  horas = prefs.getUChar("hr_h", 0);
  minutos = prefs.getUChar("hr_m", 0);
}

void Almacenamiento::encolarEvento(TipoEventoSync tipo) {
  int count = prefs.getInt("p_cnt", 0);
  if (count >= MAX_PENDIENTES) {
    Serial.println("[Almacenamiento] cola de pendientes llena, evento descartado");
    return;
  }

  String keyTipo = "p_t" + String(count);
  String keyTs = "p_ts" + String(count);
  prefs.putInt(keyTipo.c_str(), (int)tipo);
  prefs.putULong(keyTs.c_str(), (unsigned long)time(nullptr));
  prefs.putInt("p_cnt", count + 1);
}

int Almacenamiento::contarPendientes() {
  return prefs.getInt("p_cnt", 0);
}

bool Almacenamiento::obtenerPendiente(int indice, EventoPendiente &out) {
  int count = prefs.getInt("p_cnt", 0);
  if (indice < 0 || indice >= count) {
    return false;
  }

  String keyTipo = "p_t" + String(indice);
  String keyTs = "p_ts" + String(indice);
  out.tipo = (TipoEventoSync)prefs.getInt(keyTipo.c_str(), 0);
  out.timestamp = (time_t)prefs.getULong(keyTs.c_str(), 0);
  return true;
}

void Almacenamiento::eliminarPendiente(int indice) {
  int count = prefs.getInt("p_cnt", 0);
  if (indice < 0 || indice >= count) {
    return;
  }

  // Recorre el resto de la cola un lugar hacia atrás para conservar el orden.
  for (int i = indice; i < count - 1; i++) {
    EventoPendiente siguiente;
    obtenerPendiente(i + 1, siguiente);
    String keyTipo = "p_t" + String(i);
    String keyTs = "p_ts" + String(i);
    prefs.putInt(keyTipo.c_str(), (int)siguiente.tipo);
    prefs.putULong(keyTs.c_str(), (unsigned long)siguiente.timestamp);
  }

  prefs.putInt("p_cnt", count - 1);
}

void Almacenamiento::limpiarPendientes() {
  prefs.putInt("p_cnt", 0);
}

void Almacenamiento::borrarTodo() {
  prefs.clear(); // borra todas las claves del namespace "ember" (racha, hitos, hora, pendientes)
}
