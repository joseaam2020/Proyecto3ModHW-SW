# Proyecto3ModHW-SW

EMBER es un tótem interactivo diseñado para acompañar a personas en procesos de sobriedad, el proyecto 3 del curso de Modelación de Hardware y Software.

## Estructura del repo

```
test/        Sketch de prueba de hardware (e-paper, HC-SR04, touch, LED RGB, motor)
EMBER/       Firmware real: máquina de estados, racha/hitos, persistencia, etc.
tests/       Pruebas unitarias de lógica pura (Racha/Hito), corren en host
```

El detalle completo del diseño y las etapas de implementación está en `Descripcion/plan.md` (carpeta del proyecto, fuera de este repo).

## Compilar y subir el firmware (`EMBER/`)

1. Arduino IDE 2.x con el paquete de placas **ESP32 (Espressif Systems)** instalado.
2. Librería **`esp32-waveshare-epd`** instalada en `Arduino/libraries/` (ver `Descripcion/setup.md` para el wiring y la instalación).
3. `Preferences` (persistencia en flash) viene incluida con el core de ESP32 — no requiere instalación aparte.
4. Abrir `EMBER/EMBER.ino`, seleccionar **Tools > Board > ESP32 Dev Module** y el puerto COM correspondiente, y subir.
5. Abrir el **Monitor Serial** a 115200 baud para ver los cambios de estado (`[Totem] estado -> ...`) y los registros de racha.

### Modo demo

`EMBER/Config.h` tiene un flag `MODO_DEMO` (activado por defecto) que comprime los tiempos de espera de 24h a segundos, para poder ver el ciclo completo **Reposo → Riesgo/Inactividad → Reinicio** en una demo corta sin esperar un día real. La progresión de racha por toque no necesita comprimirse: cada toque exitoso ya suma un día de inmediato.

Para el comportamiento con tiempos reales, comentar la línea `#define MODO_DEMO` en `Config.h`.

## Pruebas unitarias (`tests/`)

`Racha` y `Hito` no dependen de hardware, así que se pueden probar fuera del IDE de Arduino con un compilador C++ normal:

```sh
cd tests
g++ -std=c++11 -I stubs -I ../EMBER test_racha_hito.cpp ../EMBER/Racha.cpp ../EMBER/Hito.cpp -o test_racha_hito
./test_racha_hito
```

Las transiciones de `Totem` no están en este test automatizado (está acoplado a clases de hardware) — se validan manualmente viendo el Monitor Serial en hardware real.
