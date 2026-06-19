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

    void dibujarEncabezado();
    void refrescarCompleto();          // full update: Init_Fast + Display + Sleep
    void refrescarParcialReloj();      // partial update solo del área del reloj

    // Convierte un rectángulo en coordenadas lógicas (rotation=270) a las
    // coordenadas físicas que espera EPD_4IN2_PartialDisplay (ver fórmulas
    // de rotación en Descripcion/epaper.md). Pendiente de verificar en
    // hardware real antes de la integración final (Etapa 10).
    void rectLogicoAFisico(UWORD xl0, UWORD yl0, UWORD xl1, UWORD yl1,
                            UWORD &xp0, UWORD &yp0, UWORD &xp1, UWORD &yp1);
};

#endif
