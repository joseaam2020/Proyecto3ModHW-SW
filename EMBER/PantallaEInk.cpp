#include "PantallaEInk.h"
#include "Racha.h"
#include "Hito.h"
#include <stdlib.h>
#include <string.h>

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

// Posiciones para mensaje inferior (pantalla portrait 300x400)
#define BOTTOM_MSG_Y0 300
#define BOTTOM_MSG_Y1 335

PantallaEInk::PantallaEInk()
  : buffer(nullptr), contenidoActual(""), ultimaActualizacion(0),
    modoReloj(false), horaActual(0), minutoActual(0),
    horaDibujada(255), minutoDibujada(255) {
}

// Canvas logical size (portrait)
#define CANVAS_W 300
#define CANVAS_H 400
#define CENTER_X (CANVAS_W/2)

// Heurística simple para estimar ancho en píxeles de un texto para centrarlo.
static int estimateTextWidth(const sFONT *font, const char *s) {
  size_t len = strlen(s);
  int w = 0;
  if (font == &Font24) {
    w = 18; // approx per char
  } else if (font == &Font16) {
    w = 12;
  } else if (font == &Font12) {
    w = 9;
  } else {
    w = 10;
  }
  return (int)len * w;
}

// Dibuja un rectángulo relleno usando Paint_SetPixel (por compatibilidad).
static void drawFilledRect(int x0, int y0, int w, int h, UBYTE color) {
  int xStart = x0 < 0 ? 0 : x0;
  int yStart = y0 < 0 ? 0 : y0;
  int xEnd = x0 + w;
  int yEnd = y0 + h;
  if (xEnd > CANVAS_W) xEnd = CANVAS_W;
  if (yEnd > CANVAS_H) yEnd = CANVAS_H;
  for (int xx = xStart; xx < xEnd; ++xx) {
    for (int yy = yStart; yy < yEnd; ++yy) {
      Paint_SetPixel(xx, yy, color);
    }
  }
}

// Dibuja un dígito grande estilo "seven-seg-ish" con grosor relativo.
static void drawBigDigit(int x, int y, char digit, int scale, UBYTE color) {
  // base height ~24 per unit scale
  int h = 24 * scale;
  int w = (h * 6) / 10; // aspect ratio
  int t = (h / 6 > 2) ? (h / 6) : 2; // thickness

  // segment rectangles
  int ax = x + t;
  int ay = y;
  int aw = w - 2 * t;
  int ah = t;

  int dx = x + t;
  int dy = y + h - t;
  int dw = aw;
  int dh = t;

  int fx = x;
  int fy = y + t;
  int fw = t;
  int fh = h / 2 - t;

  int bx = x + w - t;
  int by = y + t;
  int bw = t;
  int bh = h / 2 - t;

  int gx = x + t;
  int gy = y + h/2 - t/2;
  int gw = aw;
  int gh = t;

  int ex = x;
  int ey = y + h/2;
  int ew = t;
  int eh = h / 2 - t;

  // segments: A,B,C,D,E,F,G
  bool segA=false, segB=false, segC=false, segD=false, segE=false, segF=false, segG=false;
  switch (digit) {
    case '0': segA=segB=segC=segD=segE=segF=true; break;
    case '1': segB=segC=true; break;
    case '2': segA=segB=segG=segE=segD=true; break;
    case '3': segA=segB=segG=segC=segD=true; break;
    case '4': segF=segG=segB=segC=true; break;
    case '5': segA=segF=segG=segC=segD=true; break;
    case '6': segA=segF=segG=segE=segC=segD=true; break;
    case '7': segA=segB=segC=true; break;
    case '8': segA=segB=segC=segD=segE=segF=segG=true; break;
    case '9': segA=segB=segC=segD=segF=segG=true; break;
    default: break;
  }

  if (segA) drawFilledRect(ax, ay, aw, ah, color);
  if (segB) drawFilledRect(bx, by, bw, bh, color);
  if (segC) drawFilledRect(bx, by + h/2, bw, bh, color);
  if (segD) drawFilledRect(dx, dy, dw, dh, color);
  if (segE) drawFilledRect(ex, ey + t/2, ew, eh, color);
  if (segF) drawFilledRect(fx, fy, fw, fh, color);
  if (segG) drawFilledRect(gx, gy, gw, gh, color);
}

static void drawBigColon(int x, int y, int scale, UBYTE color) {
  int size = (scale * 4 > 2) ? (scale * 4) : 2;
  drawFilledRect(x, y - size - 6, size, size, color);
  drawFilledRect(x, y + 6, size, size, color);
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
  const char *hdr = "EMBER";
  int w = estimateTextWidth(&Font16, hdr);
  int x = CENTER_X - w/2;
  Paint_DrawString_EN(x, ENCABEZADO_Y, hdr, &Font16, BLACK, WHITE);
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

  // Hora en dos bloques apilados (horas arriba, minutos abajo), centrados.
  char horas[3];
  char minutos[3];
  snprintf(horas, sizeof(horas), "%02d", horaActual);
  snprintf(minutos, sizeof(minutos), "%02d", minutoActual);

  // Dibujar hora grande (scale 4) con minutos apilados debajo
  int scale = 4;
  int h = 24 * scale;
  int digitW = (h * 6) / 10;
  int spacing = scale * 6;
  int hourTotalWidth = 2 * digitW + spacing;
  int startXHour = CENTER_X - hourTotalWidth / 2;
  int hourY = 50;
  int xh = startXHour;
  drawBigDigit(xh, hourY, horas[0], scale, BLACK);
  xh += digitW + spacing;
  drawBigDigit(xh, hourY, horas[1], scale, BLACK);

  // colon to the right of the hours, vertically centered between hours and minutes
  int colonX = startXHour + hourTotalWidth + scale * 4;
  int colonY = hourY + h / 2;
  drawBigColon(colonX, colonY, scale, BLACK);

  // minutos centrados debajo de las horas
  int minuteTotalWidth = 2 * digitW + spacing;
  int startXMin = CENTER_X - minuteTotalWidth / 2;
  int minY = hourY + h + 8;
  int xm = startXMin;
  drawBigDigit(xm, minY, minutos[0], scale, BLACK);
  xm += digitW + spacing;
  drawBigDigit(xm, minY, minutos[1], scale, BLACK);

  // Mensaje de reinicio: dos líneas en la parte inferior
  const char *msg1 = "Racha reiniciada";
  const char *msg2 = "el dia uno te espera";
  int wMsg1 = estimateTextWidth(&Font16, msg1);
  int xMsg1 = CENTER_X - wMsg1/2;
  Paint_DrawString_EN(xMsg1, 300, msg1, &Font16, BLACK, WHITE);
  int wMsg2 = estimateTextWidth(&Font12, msg2);
  int xMsg2 = CENTER_X - wMsg2/2;
  Paint_DrawString_EN(xMsg2, 330, msg2, &Font12, BLACK, WHITE);

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
  // Dibujar la hora en dos bloques apilados, centrados
  char horas[3];
  char minutos[3];
  snprintf(horas, sizeof(horas), "%02d", horaActual);
  snprintf(minutos, sizeof(minutos), "%02d", minutoActual);

  // Dibujar hora grande (scale 4) con minutos apilados debajo
  int scale = 4;
  int h = 24 * scale;
  int digitW = (h * 6) / 10;
  int spacing = scale * 6;
  int hourTotalWidth = 2 * digitW + spacing;
  int startXHour = CENTER_X - hourTotalWidth / 2;
  int hourY = 50;
  int xh = startXHour;
  drawBigDigit(xh, hourY, horas[0], scale, BLACK);
  xh += digitW + spacing;
  drawBigDigit(xh, hourY, horas[1], scale, BLACK);

  // colon to the right of the hours, vertically centered between hours and minutes
  int colonX = startXHour + hourTotalWidth + scale * 4;
  int colonY = hourY + h / 2;
  drawBigColon(colonX, colonY, scale, BLACK);

  // minutos centrados debajo de las horas
  int minuteTotalWidth = 2 * digitW + spacing;
  int startXMin = CENTER_X - minuteTotalWidth / 2;
  int minY = hourY + h + 8;
  int xm = startXMin;
  drawBigDigit(xm, minY, minutos[0], scale, BLACK);
  xm += digitW + spacing;
  drawBigDigit(xm, minY, minutos[1], scale, BLACK);

  // Mensaje final siempre presente en la parte inferior
  const char *msg = "Hoy eliges bienestar";
  int wMsg = estimateTextWidth(&Font16, msg);
  int xMsg = CENTER_X - wMsg/2;
  Paint_DrawString_EN(xMsg, 320, msg, &Font16, BLACK, WHITE);

  if (contenidoActual != "reinicio") {
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
