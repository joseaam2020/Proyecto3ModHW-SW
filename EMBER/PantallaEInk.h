#ifndef PANTALLA_EINK_H
#define PANTALLA_EINK_H

#include <Arduino.h>
#include "DEV_Config.h"
#include "EPD.h"
#include "GUI_Paint.h"

// Declaración adelantada: las clases completas llegan en la Etapa 5.
// El contrato usado aquí (getDiasConsecutivos, getDiasRequeridos,
// getDescripcion) debe coincidir con esas implementaciones.
class Racha;
class Hito;

class PantallaEInk {
  public:
    PantallaEInk();
    ~PantallaEInk();

    // Inicializa el panel y limpia contenido fantasma de programas
    // anteriores (3 ciclos Init_Fast+Clear, ver Descripcion/epaper.md).
    void init();

    void mostrarProgreso(Racha &racha);
    void mostrarHito(Hito &hito);

    // No está en el diagrama de clases original: feedback visual al
    // entrar a Reinicio/Recuperación (antes no se mostraba nada).
    void mostrarReinicio();

    // mostrarReloj() no recibe parámetros (según diagrama de clases);
    // usa la hora más reciente fijada con actualizarHora(), que el
    // manejo de RTC (Etapas 7/8) llamará antes de cada refresco.
    void actualizarHora(uint8_t horas, uint8_t minutos);
    void mostrarReloj();

    String getContenidoActual() const;
    bool isModoReloj() const;

  private:
    UBYTE *buffer;
    String contenidoActual;
    unsigned long ultimaActualizacion;
    bool modoReloj;

    uint8_t horaActual;
    uint8_t minutoActual;

    // Último valor efectivamente dibujado en el panel — mostrarReloj()
    // no toca el hardware si no cambió, y así el único refresco que
    // importa (cuando sí cambia) puede ser completo sin que eso
    // signifique parpadear cada pocos segundos.
    uint8_t horaDibujada;
    uint8_t minutoDibujada;

    void dibujarEncabezado();
    void refrescarCompleto();          // full update: Init_Fast + Display + Sleep
};

#endif
