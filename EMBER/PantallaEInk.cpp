#include "PantallaEInk.h"
#include <stdlib.h>

// Layout en coordenadas LÓGICAS (canvas portrait 300×400, origen top-left,
// rotation=270 — ver Descripcion/epaper.md).
#define ENCABEZADO_X 10
#define ENCABEZADO_Y 10

#define CUERPO_X 10
#define CUERPO_Y 40

// Rectángulo del reloj: única zona que se refresca parcialmente.
#define RELOJ_X0 10
#define RELOJ_Y0 350
#define RELOJ_X1 110
#define RELOJ_Y1 390

PantallaEInk::PantallaEInk()
  : buffer(nullptr), contenidoActual(""), ultimaActualizacion(0),
    modoReloj(false), horaActual(0), minutoActual(0) {
}

PantallaEInk::~PantallaEInk() {
  if (buffer != nullptr) {
    free(buffer);
  }
}

void PantallaEInk::init() {
  DEV_Module_Init();

  UWORD imgSize = ((EPD_4IN2_WIDTH % 8 == 0)
    ? (EPD_4IN2_WIDTH / 8)
    : (EPD_4IN2_WIDTH / 8 + 1)) * EPD_4IN2_HEIGHT;
  buffer = (UBYTE *)malloc(imgSize);
  if (buffer == NULL) {
    Serial.println("[PantallaEInk] malloc failed — halting");
    while (1);
  }

  // Tres ciclos init+clear para eliminar contenido fantasma de programas
  // anteriores (ver "Ghost content" en Descripcion/epaper.md).
  for (int cp = 0; cp < 3; cp++) {
    EPD_4IN2_Init_Fast();
    EPD_4IN2_Clear();
    DEV_Delay_ms(500);
  }

  Paint_NewImage(buffer, EPD_4IN2_WIDTH, EPD_4IN2_HEIGHT, 270, WHITE);
  Paint_SelectImage(buffer);
  Paint_Clear(WHITE);
}

void PantallaEInk::dibujarEncabezado() {
  Paint_DrawString_EN(ENCABEZADO_X, ENCABEZADO_Y, "EMBER", &Font16, BLACK, WHITE);
}

void PantallaEInk::refrescarCompleto() {
  EPD_4IN2_Init_Fast();
  EPD_4IN2_Display(buffer);
  EPD_4IN2_Sleep();
  ultimaActualizacion = millis();
}

void PantallaEInk::rectLogicoAFisico(UWORD xl0, UWORD yl0, UWORD xl1, UWORD yl1,
                                      UWORD &xp0, UWORD &yp0, UWORD &xp1, UWORD &yp1) {
  // rotation=270: Xp = Yl, Yp = (EPD_4IN2_HEIGHT-1) - Xl (ver epaper.md)
  xp0 = yl0;
  xp1 = yl1;
  yp0 = (EPD_4IN2_HEIGHT - 1) - xl1;
  yp1 = (EPD_4IN2_HEIGHT - 1) - xl0;
}

void PantallaEInk::refrescarParcialReloj() {
  UWORD xp0, yp0, xp1, yp1;
  rectLogicoAFisico(RELOJ_X0, RELOJ_Y0, RELOJ_X1, RELOJ_Y1, xp0, yp0, xp1, yp1);

  EPD_4IN2_Init_Partial();
  EPD_4IN2_PartialDisplay(xp0, yp0, xp1, yp1, buffer);
  EPD_4IN2_Sleep();
  ultimaActualizacion = millis();
}

void PantallaEInk::mostrarProgreso(Racha &racha) {
  Paint_Clear(WHITE);
  dibujarEncabezado();

  char linea[32];
  snprintf(linea, sizeof(linea), "Racha: %d dias", racha.getDiasConsecutivos());
  Paint_DrawString_EN(CUERPO_X, CUERPO_Y, linea, &Font16, BLACK, WHITE);

  refrescarCompleto();
  contenidoActual = "progreso";
  modoReloj = false;
}

void PantallaEInk::mostrarHito(Hito &hito) {
  Paint_Clear(WHITE);
  dibujarEncabezado();

  char linea1[32];
  snprintf(linea1, sizeof(linea1), "Hito: %d dias!", hito.getDiasRequeridos());
  Paint_DrawString_EN(CUERPO_X, CUERPO_Y, linea1, &Font16, BLACK, WHITE);
  Paint_DrawString_EN(CUERPO_X, CUERPO_Y + 25, hito.getDescripcion().c_str(), &Font12, BLACK, WHITE);

  refrescarCompleto();
  contenidoActual = "hito";
  modoReloj = false;
}

void PantallaEInk::actualizarHora(uint8_t horas, uint8_t minutos) {
  horaActual = horas;
  minutoActual = minutos;
}

void PantallaEInk::mostrarReloj() {
  char linea[16];
  snprintf(linea, sizeof(linea), "%02d:%02d", horaActual, minutoActual);

  if (contenidoActual != "reloj") {
    // Primera vez en modo reloj: redibujar todo el fondo + reloj.
    Paint_Clear(WHITE);
    dibujarEncabezado();
    Paint_DrawString_EN(RELOJ_X0, RELOJ_Y0, linea, &Font24, BLACK, WHITE);
    refrescarCompleto();
  } else {
    // Ya estábamos en modo reloj: solo refrescar el área del reloj.
    Paint_ClearWindows(RELOJ_X0, RELOJ_Y0, RELOJ_X1, RELOJ_Y1, WHITE);
    Paint_DrawString_EN(RELOJ_X0, RELOJ_Y0, linea, &Font24, BLACK, WHITE);
    refrescarParcialReloj();
  }

  contenidoActual = "reloj";
  modoReloj = true;
}

String PantallaEInk::getContenidoActual() const {
  return contenidoActual;
}

bool PantallaEInk::isModoReloj() const {
  return modoReloj;
}
