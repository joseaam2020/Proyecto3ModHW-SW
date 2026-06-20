#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H

// Stub mínimo de Arduino.h para compilar la lógica pura (Racha/Hito) en
// host, sin el IDE de Arduino ni un ESP32 real. Solo implementa lo que
// esas clases realmente usan: la clase String.

#include <cstdint>
#include <string>

class String {
  public:
    String() {}
    String(const char *s) : data(s ? s : "") {}
    String(const std::string &s) : data(s) {}

    const char* c_str() const { return data.c_str(); }
    bool operator==(const String &other) const { return data == other.data; }
    bool operator==(const char *s) const { return data == s; }

  private:
    std::string data;
};

#endif
