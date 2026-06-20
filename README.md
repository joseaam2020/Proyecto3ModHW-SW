# Proyecto3ModHW-SW

EMBER es un tótem interactivo diseñado para acompañar a personas en procesos de sobriedad, el proyecto 3 del curso de Modelación de Hardware y Software.

## Estructura del repo

```
test/        Sketch de prueba de hardware (e-paper, HC-SR04, touch, LED RGB, motor)
EMBER/       Firmware real: máquina de estados, racha/hitos, persistencia, sync con backend
tests/       Pruebas unitarias de lógica pura (Racha/Hito), corren en host
blockcode/   App web (Etapa 11): backend Flask (blockcode/backend) + frontend (blockcode/frontend)
RT-ESP32/    Prototipo ESP32 alternativo (referencia), no es el firmware usado por el proyecto
```

El detalle completo del diseño y las etapas de implementación está en `Descripcion/plan.md` (carpeta del proyecto, fuera de este repo).

## Compilar y subir el firmware (`EMBER/`)

1. Arduino IDE 2.x con el paquete de placas **ESP32 (Espressif Systems)** instalado.
2. Librería **`esp32-waveshare-epd`** instalada en `Arduino/libraries/` (ver `Descripcion/setup.md` para el wiring y la instalación).
3. Librería **`ArduinoJson`** (v6.x) instalada vía Library Manager — la usa `ComunicacionApp` para hablar con el backend.
4. `Preferences` (persistencia en flash) y `WiFi`/`HTTPClient` vienen incluidas con el core de ESP32 — no requieren instalación aparte.
5. En `EMBER/Config.h`, configurar `WIFI_SSID`, `WIFI_PASSWORD` y `BACKEND_URL` (IP/puerto donde corre `blockcode/backend`) — ver sección "App web / backend" más abajo.
6. Abrir `EMBER/EMBER.ino`, seleccionar **Tools > Board > ESP32 Dev Module** y el puerto COM correspondiente, y subir.
7. Abrir el **Monitor Serial** a 115200 baud para ver los cambios de estado (`[Totem] estado -> ...`), los registros de racha y los intentos de sincronización (`[ComunicacionApp] ...`).

### Modo demo

`EMBER/Config.h` tiene un flag `MODO_DEMO` (activado por defecto) que comprime los tiempos de espera de 24h a segundos, para poder ver el ciclo completo **Reposo → Riesgo/Inactividad → Reinicio** en una demo corta sin esperar un día real. La progresión de racha por toque no necesita comprimirse: cada toque exitoso ya suma un día de inmediato.

Para el comportamiento con tiempos reales, comentar la línea `#define MODO_DEMO` en `Config.h`.

## App web / backend (`blockcode/`)

El tótem es la fuente de verdad de la racha — el backend solo guarda lo que el ESP32 reporta (`POST /device/sync`) y expone la hora configurada desde la app (`GET/POST /config/hora`). No recalcula días por fecha calendario para esos endpoints.

```sh
cd blockcode/backend
python -m venv .venv
./.venv/Scripts/pip install -r requirements.txt   # En Linux/Mac: .venv/bin/pip
./.venv/Scripts/python app.py                      # En Linux/Mac: .venv/bin/python
```

El servidor queda en `http://localhost:5000` (o la IP de la máquina en la red local, que es lo que hay que poner en `BACKEND_URL` de `EMBER/Config.h` y en el campo "Servidor" de `blockcode/frontend` → Perfil). Para abrir la app, basta abrir `blockcode/frontend/index.html` en un navegador.

## Pruebas unitarias (`tests/`)

`Racha` y `Hito` no dependen de hardware, así que se pueden probar fuera del IDE de Arduino con un compilador C++ normal:

```sh
cd tests
g++ -std=c++11 -I stubs -I ../EMBER test_racha_hito.cpp ../EMBER/Racha.cpp ../EMBER/Hito.cpp -o test_racha_hito
./test_racha_hito
```

Las transiciones de `Totem` no están en este test automatizado (está acoplado a clases de hardware) — se validan manualmente viendo el Monitor Serial en hardware real.
