#include "PantallaEInk.h"
#include "Racha.h"
#include "Hito.h"
#include <stdlib.h>

// Layout en coordenadas LÓGICAS (canvas portrait 300×400, origen top-left,
// rotation=270 — ver Descripcion/epaper.md).
#define ENCABEZADO_X 10
#define ENCABEZADO_Y 10

#define CUERPO_X 10
#define CUERPO_Y 40

// Posición del reloj, justo debajo del encabezado "EMBER".
#define RELOJ_X0 10
#define RELOJ_Y0 35

// Mensaje de "racha reiniciada", debajo del reloj.
#define REINICIO_MSG_X  10
#define REINICIO_MSG_Y0 75
#define REINICIO_MSG_Y1 100

PantallaEInk::PantallaEInk()
  : buffer(nullptr), contenidoActual(""), ultimaActualizacion(0),
    modoReloj(false), horaActual(0), minutoActual(0),
    horaDibujada(255), minutoDibujada(255) {
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

void PantallaEInk::mostrarReinicio() {
  // Pantalla persistente (igual que el reloj normal, pero con un mensaje
  // adicional): muestra el reloj actual + el aviso de racha reiniciada
  // juntos. Los refrescos posteriores del reloj (mostrarReloj) detectan
  // contenidoActual == "reinicio" y redibujan este mensaje también, para
  // no perderlo (el refresco es siempre completo, ver mostrarReloj()).
  char linea[16];
  snprintf(linea, sizeof(linea), "%02d:%02d", horaActual, minutoActual);

  Paint_Clear(WHITE);
  dibujarEncabezado();
  Paint_DrawString_EN(RELOJ_X0, RELOJ_Y0, linea, &Font24, BLACK, WHITE);
  Paint_DrawString_EN(REINICIO_MSG_X, REINICIO_MSG_Y0, "Racha reiniciada", &Font16, BLACK, WHITE);
  Paint_DrawString_EN(REINICIO_MSG_X, REINICIO_MSG_Y1, "El dia 1 te espera", &Font12, BLACK, WHITE);

  refrescarCompleto();
  contenidoActual = "reinicio";
  horaDibujada = horaActual;
  minutoDibujada = minutoActual;
  modoReloj = true;
}

void PantallaEInk::actualizarHora(uint8_t horas, uint8_t minutos) {
  horaActual = horas;
  minutoActual = minutos;
}

void PantallaEInk::mostrarReloj() {
  bool veniamosDeOtraPantalla = (contenidoActual != "reloj" && contenidoActual != "reinicio");

  // Si ya estábamos mostrando el reloj (o reinicio) y la hora no cambió
  // desde el último dibujo, no hay nada que hacer — no tocamos el panel.
  if (!veniamosDeOtraPantalla && horaActual == horaDibujada && minutoActual == minutoDibujada) {
    modoReloj = true;
    return;
  }

  // El refresco es SIEMPRE completo, nunca parcial: el refresco parcial
  // de este panel no es confiable (ver Descripcion/epaper.md, "Poor
  // display" es el propio comentario del fabricante) — a veces el
  // comando se envía pero el panel no voltea los píxeles, dejando un
  // valor de reloj viejo en pantalla sin avisar. Como solo se llega
  // aquí cuando el valor realmente cambió (~1 vez por minuto), el
  // parpadeo de pantalla completa no es tan frecuente como antes de
  // tener este chequeo.
  char linea[16];
  snprintf(linea, sizeof(linea), "%02d:%02d", horaActual, minutoActual);

  Paint_Clear(WHITE);
  dibujarEncabezado();
  Paint_DrawString_EN(RELOJ_X0, RELOJ_Y0, linea, &Font24, BLACK, WHITE);
  if (contenidoActual == "reinicio") {
    // Un refresco completo borra todo el buffer: hay que redibujar
    // también el aviso de racha reiniciada, o se perdería.
    Paint_DrawString_EN(REINICIO_MSG_X, REINICIO_MSG_Y0, "Racha reiniciada", &Font16, BLACK, WHITE);
    Paint_DrawString_EN(REINICIO_MSG_X, REINICIO_MSG_Y1, "El dia 1 te espera", &Font12, BLACK, WHITE);
  } else {
    contenidoActual = "reloj";
  }

  refrescarCompleto();
  horaDibujada = horaActual;
  minutoDibujada = minutoActual;
  modoReloj = true;
}

String PantallaEInk::getContenidoActual() const {
  return contenidoActual;
}

bool PantallaEInk::isModoReloj() const {
  return modoReloj;
}
