/* ────────────────────────────────────────────────────────
   EMBER — app.js
   SPA con 4 vistas. Conecta con backend Flask.
──────────────────────────────────────────────────────── */

function API() {
  return (localStorage.getItem('ember_api') || 'http://localhost:5000').replace(/\/+$/, '');
}

/* ── ESTADO LOCAL ─────────────────────────────────────── */
let state = {
  days:       0,
  best:       0,
  resets:     0,
  since:      null,
  name:       '',
  server_url: 'http://localhost:5000',
  messages:   [
    'Cada día que eliges avanzar, refuerzas quién quieres ser.',
    'La constancia es la forma más silenciosa del coraje.',
    'Hoy es suficiente. Mañana también lo será.',
  ],
  log:        [],
  totem:      { state: 'idle', last: null },

  // Minutos por "día" para el calendario del historial — debe coincidir
  // con DURACION_DIA_MS del tótem (EMBER/Config.h): 1440 (24h) en modo
  // real, o el valor en minutos usado en MODO_DEMO (ej. 1). Sin esto, en
  // demo todos los eventos caen en la misma fecha calendario y el
  // historial se ve como si todo hubiera pasado "el mismo día".
  duracion_dia_min: 1440,
};

function saveState() {
  localStorage.setItem('ember_state', JSON.stringify(state));
}
function loadState() {
  const raw = localStorage.getItem('ember_state');
  if (raw) {
    try { Object.assign(state, JSON.parse(raw)); } catch(e) { console.warn('Estado corrupto, usando default'); }
  }
}

/* ── NAVEGACIÓN ───────────────────────────────────────── */
const VIEWS = {
  'home':     'view-home',
  'history':  'view-history',
  'profile':  'view-profile',
};

function nav(to) {
  // 1. Ocultar todas las vistas
  Object.values(VIEWS).forEach(function(id) {
    var el = document.getElementById(id);
    if (el) el.classList.remove('active');
  });

  // 2. Mostrar la vista destino
  var target = document.getElementById(VIEWS[to]);
  if (target) target.classList.add('active');

  // 3. Actualizar botones del nav global
  var navBtns = document.querySelectorAll('#global-nav .nav-btn');
  navBtns.forEach(function(btn) {
    if (btn.getAttribute('data-view') === to) {
      btn.classList.add('active');
    } else {
      btn.classList.remove('active');
    }
  });

  // 4. Renderizar la vista
  if (to === 'home')       renderHome();
  if (to === 'history')    renderHistory();
  if (to === 'profile')    renderProfile();

  window.scrollTo(0, 0);
}

/* ── RENDER HOME ──────────────────────────────────────── */
// Hitos de continuidad (ver EMBER/Config.h: HITO_DIAS_1/2/3).
var HITOS = [7, 30, 90];

function proximoHito(dias) {
  for (var i = 0; i < HITOS.length; i++) {
    if (dias < HITOS[i]) return HITOS[i];
  }
  return null; // ya se alcanzaron todos los hitos
}

function renderHome() {
  document.getElementById('home-days').textContent = state.days;

  var hito = proximoHito(state.days);
  if (hito === null) {
    document.getElementById('home-progress').style.width = '100%';
    document.getElementById('home-progress-label').textContent =
      state.days + ' días — todos los hitos alcanzados';
  } else {
    var pct = (state.days / hito) * 100;
    document.getElementById('home-progress').style.width = pct + '%';
    document.getElementById('home-progress-label').textContent =
      state.days + ' / ' + hito + ' días para el próximo hito';
  }

  // Mensaje del día
  var msgs = state.messages.length ? state.messages : ['Sigue adelante.'];
  var idx  = (new Date().getDate()) % msgs.length;
  document.getElementById('home-msg').textContent = msgs[idx];

  // Tótem
  document.getElementById('totem-state-label').textContent = totemStateLabel(state.totem.state);
  document.getElementById('totem-last').textContent =
    state.totem.last ? ('Último contacto: ' + fmtDatetime(state.totem.last)) : 'Sin contacto reciente';
}

function totemStateLabel(s) {
  var MAP = { idle: 'En reposo', active: 'Activo', contact: 'Contacto detectado',
              celebration: 'Celebración', risk: 'En riesgo' };
  return MAP[s] || 'Desconocido';
}

/* ── SYNC TÓTEM ──────────────────────────────────────── */
function syncTotem() {
  fetch(API() + '/status')
  .then(function(res) { return res.json(); })
  .then(function(data) {
    state.totem.state = data.totem_state || 'idle';
    state.totem.last  = data.last_contact;
    if (typeof data.days === 'number') state.days = data.days;
    if (typeof data.best === 'number') state.best = data.best;
    if (typeof data.resets === 'number') state.resets = data.resets;
    saveState();
    renderHome();
    showToast('Tótem sincronizado');
  })
  .catch(function() {
    showToast('Sin conexión al servidor');
  });
}

/* ── RENDER HISTORIAL ─────────────────────────────────── */
// type esperado en cada entrada de state.log (mismo vocabulario que
// /history del backend): "checkin" | "hito" | "reset"/"reinicio" |
// otros (touch/proximity, se ignoran para el calendario).
function esIngreso(tipo)  { return tipo === 'checkin' || tipo === 'hito'; }
function esReinicio(tipo) { return tipo === 'reset' || tipo === 'reinicio'; }

function renderHistory() {
  document.getElementById('h-total-days').textContent = state.days;
  document.getElementById('h-best').textContent       = state.best;
  document.getElementById('h-resets').textContent      = state.resets;
  renderCalendar();
  renderLog();

  // El tótem es quien registra los ingresos/reinicios reales — el
  // calendario y el contador de reinicios deben reflejar /history, no
  // solo lo que esta misma sesión de la app generó localmente.
  fetch(API() + '/history?limit=500')
  .then(function(res) { return res.json(); })
  .then(function(data) {
    if (!data.ok) return;
    state.resets = data.resets;
    state.log    = data.entries; // [{ts, type, detail, date}]
    saveState();
    document.getElementById('h-resets').textContent = state.resets;
    renderCalendar();
    renderLog();
  })
  .catch(function() {});
}

// Agrupa por VENTANAS DE TIEMPO de duracion_dia_min minutos (no por
// fecha calendario): con MODO_DEMO en el tótem, un "día" dura 1 min
// real, así que agrupar por fecha calendario metía decenas de "días"
// de racha en la misma celda. Usamos el "ts" completo (fecha+hora) de
// cada entrada de log, no solo su "date", para poder ubicarla en la
// ventana correcta.
function renderCalendar() {
  var grid     = document.getElementById('cal-grid');
  var ahora    = new Date();
  var durMs    = duracionDiaMin() * 60 * 1000;
  var n        = 56;

  var label = document.getElementById('cal-section-label');
  label.textContent = (duracionDiaMin() === 1440)
    ? 'Últimas 8 semanas'
    : ('Últimos ' + n + ' períodos de ' + duracionDiaMin() + ' min');

  grid.innerHTML = '';
  for (var i = n - 1; i >= 0; i--) {
    var finVentana    = new Date(ahora.getTime() - i * durMs);
    var inicioVentana = new Date(finVentana.getTime() - durMs);

    var deEsaVentana = state.log.filter(function(l) {
      var t = new Date(l.ts || l.date).getTime();
      return t > inicioVentana.getTime() && t <= finVentana.getTime();
    });
    var huboReinicio = deEsaVentana.some(function(l) { return esReinicio(l.type); });
    var huboIngreso  = deEsaVentana.some(function(l) { return esIngreso(l.type); });

    var cell = document.createElement('div');
    cell.className = 'cal-cell';
    var esVentanaActual = (i === 0);
    if (huboReinicio) {
      cell.classList.add('reset');       // rojo: se perdió la racha en esa ventana
    } else if (huboIngreso) {
      cell.classList.add('ok');          // negro: se hizo ingreso en esa ventana
    } else if (!esVentanaActual) {
      cell.classList.add('miss');        // gris: no se hizo ingreso
    }
    if (esVentanaActual) cell.classList.add('today');
    cell.title = inicioVentana.toLocaleString('es') + ' – ' + finVentana.toLocaleString('es');
    grid.appendChild(cell);
  }
}

function duracionDiaMin() {
  return state.duracion_dia_min || 1440;
}

function renderLog() {
  var ul = document.getElementById('log-list');
  var entries = state.log.slice(0, 20);
  if (!entries.length) {
    ul.innerHTML = '<li class="log-item"><span style="color:var(--ash)">Sin registros aún</span></li>';
    return;
  }
  ul.innerHTML = entries.map(function(e) {
    var label, cls, badge;
    if (esReinicio(e.type)) {
      label = 'Reinicio'; cls = 'reset'; badge = 'Reinicio';
    } else if (e.type === 'hito') {
      label = 'Hito alcanzado'; cls = 'ok'; badge = 'Hito';
    } else if (e.type === 'checkin') {
      label = 'Check-in'; cls = 'ok'; badge = 'Registrado';
    } else {
      label = 'Sin registro'; cls = 'miss'; badge = 'Perdido';
    }
    return '<li class="log-item">' +
      '<span>' + label + '</span>' +
      '<span class="log-date">' + fmtDate(e.date) + '</span>' +
      '<span class="log-badge ' + cls + '">' + badge + '</span>' +
    '</li>';
  }).join('');
}

/* ── RENDER PERFIL ────────────────────────────────────── */
function renderProfile() {
  var initials = (state.name || '—').slice(0, 2).toUpperCase();
  document.getElementById('profile-initials').textContent = initials;
  document.getElementById('profile-name-label').textContent = state.name || '—';
  document.getElementById('profile-since').textContent = state.since ? fmtDate(state.since) : '—';
  document.getElementById('inp-name').value    = state.name || '';
  document.getElementById('inp-server').value  = state.server_url || 'http://localhost:5000';
  document.getElementById('ptotem-state').textContent = totemStateLabel(state.totem.state);
  document.getElementById('ptotem-last').textContent  = state.totem.last ? fmtDatetime(state.totem.last) : '—';
  document.getElementById('ptotem-ip').textContent    = state.server_url || '—';

  // Precarga la hora actual del dispositivo como punto de partida —
  // el usuario solo necesita confirmar (o ajustar) y enviar.
  var ahora = new Date();
  document.getElementById('inp-hora').value =
    String(ahora.getHours()).padStart(2, '0') + ':' + String(ahora.getMinutes()).padStart(2, '0');

  document.getElementById('inp-duracion-dia').value = duracionDiaMin();

  renderMsgs();
}

function renderMsgs() {
  var list = document.getElementById('msgs-list');
  list.innerHTML = state.messages.map(function(m, i) {
    return '<div class="msg-item">' +
      '<span>' + m + '</span>' +
      '<button class="msg-del" onclick="deleteMsg(' + i + ')">✕</button>' +
    '</div>';
  }).join('');
}

function deleteMsg(i) {
  state.messages.splice(i, 1);
  saveState();
  renderMsgs();
  pushMessages();
}

function addMessage() {
  var inp = document.getElementById('inp-new-msg');
  var txt = inp.value.trim();
  if (!txt) return;
  state.messages.push(txt);
  inp.value = '';
  saveState();
  renderMsgs();
  pushMessages();
}

function pushMessages() {
  fetch(API() + '/messages', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ messages: state.messages })
  }).catch(function() {});
}

function saveProfile() {
  state.name       = document.getElementById('inp-name').value.trim();
  state.server_url = document.getElementById('inp-server').value.trim() || 'http://localhost:5000';
  if (!state.since && state.name) state.since = todayISO();
  localStorage.setItem('ember_api', state.server_url);
  saveState();
  renderProfile();
  showToast('Perfil guardado');
}

function setHoraTotem() {
  var valor = document.getElementById('inp-hora').value; // "HH:MM"
  if (!valor) { showToast('Elige una hora primero'); return; }

  var partes  = valor.split(':');
  var horas   = parseInt(partes[0], 10);
  var minutos = parseInt(partes[1], 10);

  fetch(API() + '/config/hora', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ horas: horas, minutos: minutos })
  })
  .then(function(res) { return res.json(); })
  .then(function(data) {
    if (data.ok) {
      showToast('Hora del tótem configurada: ' + valor);
    } else {
      showToast(data.error || 'Error al configurar la hora');
    }
  })
  .catch(function() {
    showToast('Sin conexión al servidor');
  });
}

function setDuracionDia() {
  var val = parseInt(document.getElementById('inp-duracion-dia').value, 10);
  if (!val || val < 1) { showToast('Pon un número de minutos válido'); return; }
  state.duracion_dia_min = val;
  saveState();
  showToast('Calendario ajustado a ' + val + ' min/día');
}

function borrarMemoriaTotem() {
  var ok = confirm(
    '¿Borrar toda la memoria del tótem?\n\n' +
    'Esto reinicia la racha, los hitos y la hora guardados en el ESP32, ' +
    'y borra el historial del backend. No se puede deshacer.'
  );
  if (!ok) return;

  fetch(API() + '/device/wipe', { method: 'POST' })
  .then(function(res) { return res.json(); })
  .then(function(data) {
    if (!data.ok) { showToast(data.error || 'No se pudo borrar la memoria'); return; }

    state.days   = 0;
    state.best   = 0;
    state.resets = 0;
    state.since  = null;
    state.log    = [];
    state.totem.state = 'idle';
    state.totem.last  = null;
    saveState();
    renderProfile();
    showToast('Memoria borrada. El tótem se actualizará en su próximo sync.');
  })
  .catch(function() {
    showToast('Sin conexión al servidor');
  });
}

/* ── UTILIDADES ───────────────────────────────────────── */
function todayISO() {
  return toISO(new Date());
}

function toISO(d) {
  var y = d.getFullYear();
  var m = String(d.getMonth() + 1).padStart(2, '0');
  var day = String(d.getDate()).padStart(2, '0');
  return y + '-' + m + '-' + day;
}

function fmtDate(iso) {
  if (!iso) return '—';
  var parts = iso.split('-');
  return parts[2] + '/' + parts[1] + '/' + parts[0];
}

function fmtDatetime(iso) {
  if (!iso) return '—';
  var d = new Date(iso);
  return d.toLocaleDateString('es', { day: '2-digit', month: 'short' }) +
    ', ' + d.toLocaleTimeString('es', { hour: '2-digit', minute: '2-digit' });
}

function showToast(msg, ms) {
  ms = ms || 2800;
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.classList.add('show');
  setTimeout(function() { t.classList.remove('show'); }, ms);
}

/* ── POLLING TÓTEM ───────────────────────────────────── */
function pollTotem() {
  fetch(API() + '/status')
  .then(function(res) { return res.json(); })
  .then(function(data) {
    state.totem.state = data.totem_state || 'idle';
    state.totem.last  = data.last_contact;
    if (typeof data.days === 'number') state.days = data.days;
    if (typeof data.best === 'number') state.best = data.best;
    if (typeof data.resets === 'number') state.resets = data.resets;
    saveState();
  })
  .catch(function() {});
}

/* ── ARRANQUE ─────────────────────────────────────────── */
loadState();
nav('home');
setInterval(pollTotem, 30000);