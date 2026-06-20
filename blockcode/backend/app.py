"""
EMBER — Flask backend
─────────────────────────────────────────────────────────
Gestiona el estado de sobriedad/continuidad del tótem EMBER.

Endpoints:
  GET  /status          → estado actual (días, tótem, etc.)
  POST /checkin         → registra un día de continuidad (check-in manual desde la app)
  POST /reset           → reinicia la racha (manual desde la app)
  POST /touch           → el ESP32 avisa de contacto táctil
  POST /proximity       → el ESP32 avisa de detección de proximidad
  POST /device/sync     → el ESP32 (Totem) reporta su racha real (days/totem_state/event).
                           El tótem es la fuente de verdad: este endpoint NO recalcula
                           días por fecha calendario, solo guarda lo que reporta.
  GET  /config/hora     → hora configurada para el tótem, proyectada a "ahora"
  POST /config/hora     → configura la hora del tótem desde la app {horas, minutos}
  POST /device/wipe     → solicita borrar la memoria del tótem (botón en Perfil)
  GET  /device/wipe     → el ESP32 consulta si hay un borrado pendiente
  POST /device/wipe/ack → el ESP32 confirma que ya borró su flash
  GET  /messages        → lista de mensajes del día
  POST /messages        → actualiza los mensajes del día
  GET  /history         → historial completo
  GET  /today_message   → mensaje del día actual

Persistencia:
  data/streak.txt    → racha actual, mejor racha, reinicios, fecha de inicio
  data/log.txt       → log de eventos (checkin, reset, touch, proximity, device_sync)
  data/messages.txt  → mensajes motivacionales (uno por línea)
  data/config.txt    → offset de hora configurada para el tótem
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import pathlib, datetime, json, threading

app   = Flask(__name__)
CORS(app)

# ── Directorios de datos ────────────────────────────────
BASE      = pathlib.Path(__file__).parent.parent / "data"
BASE.mkdir(parents=True, exist_ok=True)

STREAK_FILE  = BASE / "streak.txt"
LOG_FILE     = BASE / "log.txt"
MESSAGES_FILE= BASE / "messages.txt"
CONFIG_FILE  = BASE / "config.txt"
WIPE_FLAG_FILE = BASE / "wipe.flag"

# ── Estado en memoria (caché) ───────────────────────────
_lock  = threading.Lock()
_state = {
    "days":           0,
    "best":           0,
    "resets":         0,
    "since":          None,          # ISO date string
    "totem_state":    "idle",        # idle | active | contact | risk | celebration
    "last_contact":   None,          # ISO datetime string
    "pending_checkin":False,         # el ESP32 activó un check-in físico
    "proximity":      False,         # sensor de proximidad activo
}

# ── Estados del tótem ───────────────────────────────────
TOTEM_STATES = {
    "idle":        {"color": "#4B5563", "vibration": "none",   "description": "En reposo"},
    "active":      {"color": "#FF8C42", "vibration": "soft",   "description": "Activo"},
    "contact":     {"color": "#FF3C00", "vibration": "strong", "description": "Contacto"},
    "risk":        {"color": "#F59E0B", "vibration": "pulse",  "description": "En riesgo"},
    "celebration": {"color": "#A855F7", "vibration": "burst",  "description": "Celebración"},
}

MILESTONE_DAYS = {7, 14, 21, 30, 60, 90, 180, 365}

DEFAULT_MESSAGES = [
    "Cada día que eliges avanzar, refuerzas quién quieres ser.",
    "La constancia es la forma más silenciosa del coraje.",
    "Hoy es suficiente. Mañana también lo será.",
    "Tu proceso importa más que la velocidad.",
    "Un día a la vez. Eso es todo lo que se pide.",
    "No has llegado hasta aquí para rendirte hoy.",
    "El tótem recuerda cada vez que elegiste continuar.",
]


# ── I/O de archivos ─────────────────────────────────────
def _read_streak():
    """Lee streak.txt → dict con days, best, resets, since."""
    if not STREAK_FILE.exists():
        return {"days": 0, "best": 0, "resets": 0, "since": None}
    try:
        data = json.loads(STREAK_FILE.read_text(encoding="utf-8"))
        return {
            "days":   int(data.get("days", 0)),
            "best":   int(data.get("best", 0)),
            "resets": int(data.get("resets", 0)),
            "since":  data.get("since"),
        }
    except Exception:
        return {"days": 0, "best": 0, "resets": 0, "since": None}


def _write_streak(days, best, resets, since):
    STREAK_FILE.write_text(
        json.dumps({"days": days, "best": best, "resets": resets, "since": since},
                   ensure_ascii=False, indent=2),
        encoding="utf-8"
    )


def _append_log(event_type: str, detail: str = ""):
    """Añade una línea al log.txt: ISO_DATETIME | type | detail"""
    ts  = datetime.datetime.now().isoformat(timespec="seconds")
    line = f"{ts} | {event_type} | {detail}\n"
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(line)


def _read_log(limit=100):
    """Lee log.txt y devuelve lista de dicts [{ts, type, detail}]."""
    if not LOG_FILE.exists():
        return []
    lines = LOG_FILE.read_text(encoding="utf-8").strip().splitlines()
    entries = []
    for line in reversed(lines[-limit:]):
        parts = line.split(" | ", 2)
        if len(parts) >= 2:
            entries.append({
                "ts":     parts[0],
                "type":   parts[1],
                "detail": parts[2] if len(parts) > 2 else "",
                "date":   parts[0][:10],
            })
    return entries


def _read_messages():
    if not MESSAGES_FILE.exists():
        _write_messages(DEFAULT_MESSAGES)
        return DEFAULT_MESSAGES
    lines = [l.strip() for l in
             MESSAGES_FILE.read_text(encoding="utf-8").splitlines() if l.strip()]
    return lines if lines else DEFAULT_MESSAGES


def _write_messages(msgs: list):
    MESSAGES_FILE.write_text(
        "\n".join(m.strip() for m in msgs if m.strip()),
        encoding="utf-8"
    )


def _minutos_desde_medianoche(dt):
    return dt.hour * 60 + dt.minute


def _read_config_offset():
    """Lee el offset (minutos) guardado, o None si nunca se configuró."""
    if not CONFIG_FILE.exists():
        return None
    try:
        data = json.loads(CONFIG_FILE.read_text(encoding="utf-8"))
        return int(data["offset_min"])
    except Exception:
        return None


def _write_config_offset(offset_min):
    CONFIG_FILE.write_text(
        json.dumps({"offset_min": offset_min}, ensure_ascii=False),
        encoding="utf-8"
    )


def _has_checked_today():
    today = datetime.date.today().isoformat()
    log   = _read_log(10)
    return any(e["type"] == "checkin" and e["date"] == today for e in log)


def _today_message():
    msgs = _read_messages()
    idx  = datetime.date.today().toordinal() % len(msgs)
    return msgs[idx]


def _determine_totem_state(days):
    """Decide el estado visual del tótem según la racha."""
    if days in MILESTONE_DAYS:
        return "celebration"
    if days == 0:
        return "idle"
    # Si llevan más de 2 días sin check-in → risk
    log   = _read_log(5)
    today = datetime.date.today()
    if log:
        last_checkin = next((e for e in log if e["type"] == "checkin"), None)
        if last_checkin:
            last_date = datetime.date.fromisoformat(last_checkin["date"])
            if (today - last_date).days >= 2:
                return "risk"
    return "active"


def _load_state_from_disk():
    streak = _read_streak()
    with _lock:
        _state["days"]   = streak["days"]
        _state["best"]   = streak["best"]
        _state["resets"] = streak["resets"]
        _state["since"]  = streak["since"]
        _state["totem_state"] = _determine_totem_state(streak["days"])


# ── Endpoints ───────────────────────────────────────────
@app.route("/status", methods=["GET"])
def status():
    with _lock:
        return jsonify({
            "ok":              True,
            "days":            _state["days"],
            "best":            _state["best"],
            "resets":          _state["resets"],
            "since":           _state["since"],
            "totem_state":     _state["totem_state"],
            "totem_info":      TOTEM_STATES.get(_state["totem_state"], {}),
            "last_contact":    _state["last_contact"],
            "pending_checkin": _state["pending_checkin"],
            "proximity":       _state["proximity"],
            "today_message":   _today_message(),
        })


@app.route("/checkin", methods=["POST"])
def checkin():
    today = datetime.date.today().isoformat()
    with _lock:
        if _has_checked_today():
            return jsonify({
                "ok":    False,
                "error": "Ya se registró check-in hoy",
                "days":  _state["days"],
                "best":  _state["best"],
            }), 409

        _state["days"] += 1
        if _state["days"] > _state["best"]:
            _state["best"] = _state["days"]
        if not _state["since"]:
            _state["since"] = today
        _state["pending_checkin"] = False
        _state["last_contact"]    = datetime.datetime.now().isoformat(timespec="seconds")

        totem_s = _determine_totem_state(_state["days"])
        _state["totem_state"] = totem_s

        _write_streak(_state["days"], _state["best"], _state["resets"], _state["since"])
        _append_log("checkin", f"day={_state['days']}")

        return jsonify({
            "ok":          True,
            "days":        _state["days"],
            "best":        _state["best"],
            "totem_state": totem_s,
            "totem_info":  TOTEM_STATES[totem_s],
            "milestone":   _state["days"] in MILESTONE_DAYS,
            "message":     _today_message(),
        })


@app.route("/reset", methods=["POST"])
def reset():
    data   = request.get_json(silent=True) or {}
    reason = data.get("reason", "manual")
    with _lock:
        prev = _state["days"]
        _state["days"]    = 0
        _state["since"]   = None
        _state["resets"] += 1
        _state["pending_checkin"] = False
        _state["totem_state"]     = "idle"

        _write_streak(0, _state["best"], _state["resets"], None)
        _append_log("reset", f"prev_days={prev} reason={reason}")

        return jsonify({
            "ok":      True,
            "days":    0,
            "resets":  _state["resets"],
            "message": "Racha reiniciada. Nuevo comienzo.",
        })


@app.route("/touch", methods=["POST"])
def touch():
    """El ESP32 llama a este endpoint cuando el sensor táctil detecta contacto."""
    now = datetime.datetime.now().isoformat(timespec="seconds")
    with _lock:
        _state["last_contact"]    = now
        _state["pending_checkin"] = True
        _state["totem_state"]     = "contact"
        _append_log("touch", f"ts={now}")

        return jsonify({
            "ok":          True,
            "totem_state": "contact",
            "action":      "checkin_pending",
        })


@app.route("/proximity", methods=["POST"])
def proximity():
    """El ESP32 avisa cuando el sensor de proximidad detecta presencia."""
    data    = request.get_json(silent=True) or {}
    active  = bool(data.get("active", True))
    with _lock:
        _state["proximity"]   = active
        if active:
            # Aumentar luminosidad: estado pasa a active si estaba idle
            if _state["totem_state"] == "idle":
                _state["totem_state"] = "active"
        else:
            if _state["totem_state"] == "active":
                _state["totem_state"] = "idle"
        _append_log("proximity", f"active={active}")

        return jsonify({
            "ok":          True,
            "proximity":   active,
            "totem_state": _state["totem_state"],
        })


@app.route("/device/sync", methods=["POST"])
def device_sync():
    """El ESP32 (Totem) reporta su racha real tras un toque, hito o
    reinicio. A diferencia de /checkin, este endpoint NO recalcula días
    por fecha calendario — el tótem ya decidió eso localmente (incluye
    MODO_DEMO, donde un "día" no equivale a un día calendario)."""
    data = request.get_json(silent=True) or {}
    try:
        days = int(data["days"])
    except (KeyError, TypeError, ValueError):
        return jsonify({"ok": False, "error": "days es requerido (entero)"}), 400

    totem_state = data.get("totem_state", _state["totem_state"])
    if totem_state not in TOTEM_STATES:
        totem_state = _state["totem_state"]
    evento = str(data.get("event", ""))

    with _lock:
        _state["days"] = days
        if days > _state["best"]:
            _state["best"] = days
        if days > 0 and not _state["since"]:
            _state["since"] = datetime.date.today().isoformat()
        if days == 0:
            _state["since"] = None
        if evento == "reinicio":
            _state["resets"] += 1
        _state["totem_state"]  = totem_state
        _state["last_contact"] = datetime.datetime.now().isoformat(timespec="seconds")
        _state["pending_checkin"] = False

        _write_streak(_state["days"], _state["best"], _state["resets"], _state["since"])
        # Se usa "evento" (checkin|hito|reinicio) como type del log, en vez de
        # un "device_sync" genérico, para que /history quede en el mismo
        # vocabulario que ya usan /checkin ("checkin") y /reset ("reset") —
        # así el frontend puede armar el calendario con una sola regla.
        _append_log(evento or "device_sync", f"days={days} state={totem_state}")

        return jsonify({
            "ok":          True,
            "days":        _state["days"],
            "best":        _state["best"],
            "totem_state": _state["totem_state"],
        })


@app.route("/config/hora", methods=["GET"])
def get_hora():
    """Hora configurada para el tótem, proyectada al momento actual del
    servidor — así el ESP32 puede aplicarla en cada poll sin riesgo de
    congelar su reloj (ver Totem::establecerHora())."""
    offset_min = _read_config_offset()
    if offset_min is None:
        return jsonify({"ok": False, "error": "hora no configurada"}), 404

    actual = (_minutos_desde_medianoche(datetime.datetime.now()) + offset_min) % 1440
    return jsonify({"ok": True, "horas": actual // 60, "minutos": actual % 60})


@app.route("/config/hora", methods=["POST"])
def set_hora():
    """Configura la hora del tótem desde la app: {horas, minutos}."""
    data = request.get_json(silent=True) or {}
    try:
        horas   = int(data["horas"])
        minutos = int(data["minutos"])
    except (KeyError, TypeError, ValueError):
        return jsonify({"ok": False, "error": "horas y minutos son requeridos (enteros)"}), 400
    if not (0 <= horas <= 23 and 0 <= minutos <= 59):
        return jsonify({"ok": False, "error": "horas debe ser 0-23 y minutos 0-59"}), 400

    objetivo   = horas * 60 + minutos
    offset_min = (objetivo - _minutos_desde_medianoche(datetime.datetime.now())) % 1440
    _write_config_offset(offset_min)

    return jsonify({"ok": True, "horas": horas, "minutos": minutos})


@app.route("/device/wipe", methods=["POST"])
def device_wipe_request():
    """Botón "Borrar memoria del tótem" en Perfil: limpia de inmediato el
    estado/historial del backend, y deja una marca para que el ESP32
    borre su flash (racha, hitos, hora) la próxima vez que sincronice
    (ver Totem::sincronizarConApp())."""
    with _lock:
        _state["days"]            = 0
        _state["best"]            = 0
        _state["resets"]          = 0
        _state["since"]           = None
        _state["totem_state"]     = "idle"
        _state["last_contact"]    = None
        _state["pending_checkin"] = False

        _write_streak(0, 0, 0, None)
        LOG_FILE.write_text("", encoding="utf-8")
        if CONFIG_FILE.exists():
            CONFIG_FILE.unlink()
        WIPE_FLAG_FILE.write_text("1", encoding="utf-8")

    return jsonify({"ok": True})


@app.route("/device/wipe", methods=["GET"])
def device_wipe_status():
    """El ESP32 consulta si hay un borrado de flash pendiente de aplicar."""
    return jsonify({"ok": True, "pending": WIPE_FLAG_FILE.exists()})


@app.route("/device/wipe/ack", methods=["POST"])
def device_wipe_ack():
    """El ESP32 confirma que ya borró su flash — sin esto, seguiría
    borrándose en cada ciclo de sincronización para siempre."""
    if WIPE_FLAG_FILE.exists():
        WIPE_FLAG_FILE.unlink()
    return jsonify({"ok": True})


@app.route("/messages", methods=["GET"])
def get_messages():
    return jsonify({"ok": True, "messages": _read_messages()})


@app.route("/messages", methods=["POST"])
def set_messages():
    data = request.get_json(silent=True) or {}
    msgs = data.get("messages", [])
    if not isinstance(msgs, list):
        return jsonify({"ok": False, "error": "messages debe ser una lista"}), 400
    _write_messages(msgs)
    return jsonify({"ok": True, "count": len(msgs)})


@app.route("/history", methods=["GET"])
def history():
    limit = min(int(request.args.get("limit", 50)), 500)
    return jsonify({
        "ok":      True,
        "entries": _read_log(limit),
        "days":    _state["days"],
        "best":    _state["best"],
        "resets":  _state["resets"],
    })


@app.route("/today_message", methods=["GET"])
def today_message():
    return jsonify({"ok": True, "message": _today_message()})


@app.route("/health", methods=["GET"])
def health():
    return jsonify({
        "ok":           True,
        "streak_file":  str(STREAK_FILE),
        "log_file":     str(LOG_FILE),
        "messages_file":str(MESSAGES_FILE),
        "data_dir":     str(BASE),
        "data_exists":  BASE.exists(),
    })


# ── Arranque ─────────────────────────────────────────────
if __name__ == "__main__":
    _load_state_from_disk()
    print("=" * 55)
    print("  EMBER backend")
    print(f"  Datos en: {BASE}")
    print(f"  Racha actual: {_state['days']} días")
    print("=" * 55)
    app.run(host="0.0.0.0", port=5000, debug=True)