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
  checkedToday: false,
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
  'check-in': 'view-check-in',
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
  if (to === 'check-in')   renderCheckin();
  if (to === 'history')    renderHistory();
  if (to === 'profile')    renderProfile();

  window.scrollTo(0, 0);
}

/* ── RENDER HOME ──────────────────────────────────────── */
function renderHome() {
  document.getElementById('home-days').textContent = state.days;

  var weekDays = getWeekDays();
  var checked  = weekDays.filter(function(d) { return isChecked(d); }).length;
  var pct = weekDays.length ? (checked / weekDays.length) * 100 : 0;
  document.getElementById('home-progress').style.width = pct + '%';
  document.getElementById('home-progress-label').textContent =
    checked + ' / ' + weekDays.length + ' días esta semana';

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

/* ── RENDER CHECK-IN ──────────────────────────────────── */
function renderCheckin() {
  document.getElementById('orb-days').textContent = state.days;

  var alreadyDone = hasCheckedToday();
  var btn = document.getElementById('btn-checkin');
  btn.disabled = alreadyDone;
  btn.textContent = alreadyDone ? '✓ Registrado hoy' : 'Registrar hoy';
  btn.style.opacity = alreadyDone ? '.5' : '1';
  btn.style.cursor  = alreadyDone ? 'default' : 'pointer';

  document.getElementById('checkin-msg').innerHTML = alreadyDone
    ? 'Ya registraste tu presencia hoy.<br>Hasta mañana.'
    : 'Coloca tu mano sobre el tótem<br>o registra manualmente';

  setStateBadge(state.totem.state === 'risk' ? 'risk' : 'ok');
}

function setStateBadge(type) {
  var badge = document.getElementById('state-badge');
  var text  = { ok: 'Continuidad', risk: 'En riesgo',
                restart: 'Reinicio', celebrate: 'Celebración' };
  badge.className = 'state-badge state-' + type;
  document.getElementById('state-badge-text').textContent = text[type] || 'Continuidad';
}

/* ── CHECK-IN ACTION ─────────────────────────────────── */
function doCheckin() {
  if (hasCheckedToday()) { showToast('Ya registraste hoy'); return; }

  fetch(API() + '/checkin', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({})
  })
  .then(function(res) { return res.json(); })
  .then(function(data) {
    if (data.ok) {
      state.days         = data.days;
      state.best         = data.best;
      state.checkedToday = true;
      state.totem.state  = data.totem_state || 'active';
      state.totem.last   = new Date().toISOString();
      state.log.unshift({ date: todayISO(), type: 'ok' });
      saveState();
      renderCheckin();
      showToast('✓ Día ' + state.days + ' registrado');
      animateCheckin();
    } else {
      showToast(data.error || 'Error al registrar');
    }
  })
  .catch(function() {
    // Offline: actualizar localmente
    state.days++;
    if (state.days > state.best) state.best = state.days;
    state.checkedToday = true;
    state.log.unshift({ date: todayISO(), type: 'ok' });
    state.totem.last = new Date().toISOString();
    saveState();
    renderCheckin();
    showToast('✓ Día ' + state.days + ' (sin conexión al servidor)');
    animateCheckin();
  });
}

function animateCheckin() {
  var core = document.getElementById('orb-core');
  core.style.transform = 'scale(1.18)';
  core.style.transition = 'transform .3s ease';
  setTimeout(function() { core.style.transform = 'scale(1)'; }, 400);
}

/* ── RESET ─────────────────────────────────────────────── */
function showResetConfirm() { document.getElementById('modal-reset').style.display = 'flex'; }
function hideResetConfirm() { document.getElementById('modal-reset').style.display = 'none'; }

function doReset() {
  hideResetConfirm();
  fetch(API() + '/reset', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({})
  })
  .then(function(res) { return res.json(); })
  .then(function() {
    applyReset();
    showToast('Racha reiniciada. Nuevo comienzo.');
  })
  .catch(function() {
    applyReset();
    showToast('Racha reiniciada (sin conexión)');
  });
}

function applyReset() {
  state.days = 0;
  state.resets++;
  state.checkedToday = false;
  state.log.unshift({ date: todayISO(), type: 'reset' });
  saveState();
  renderCheckin();
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
    saveState();
    renderHome();
    showToast('Tótem sincronizado');
  })
  .catch(function() {
    showToast('Sin conexión al servidor');
  });
}

/* ── RENDER HISTORIAL ─────────────────────────────────── */
function renderHistory() {
  document.getElementById('h-total-days').textContent = state.days;
  document.getElementById('h-best').textContent       = state.best;
  document.getElementById('h-resets').textContent      = state.resets;
  renderCalendar();
  renderLog();
}

function renderCalendar() {
  var grid  = document.getElementById('cal-grid');
  var today = new Date();
  var cells = [];
  for (var i = 55; i >= 0; i--) {
    var d = new Date(today);
    d.setDate(d.getDate() - i);
    cells.push(d);
  }
  grid.innerHTML = '';
  cells.forEach(function(d) {
    var iso   = toISO(d);
    var entry = state.log.find(function(l) { return l.date === iso; });
    var cell  = document.createElement('div');
    cell.className = 'cal-cell';
    if (entry) {
      cell.classList.add(entry.type === 'reset' ? 'reset' : 'ok');
    } else if (d <= today) {
      cell.classList.add('miss');
    }
    if (iso === todayISO()) cell.classList.add('today');
    cell.title = iso;
    grid.appendChild(cell);
  });
}

function renderLog() {
  var ul = document.getElementById('log-list');
  var entries = state.log.slice(0, 20);
  if (!entries.length) {
    ul.innerHTML = '<li class="log-item"><span style="color:var(--ash)">Sin registros aún</span></li>';
    return;
  }
  ul.innerHTML = entries.map(function(e) {
    var label  = e.type === 'ok' ? 'Check-in' : (e.type === 'reset' ? 'Reinicio' : 'Sin registro');
    var badge  = e.type === 'ok' ? 'Registrado' : (e.type === 'reset' ? 'Reinicio' : 'Perdido');
    return '<li class="log-item">' +
      '<span>' + label + '</span>' +
      '<span class="log-date">' + fmtDate(e.date) + '</span>' +
      '<span class="log-badge ' + e.type + '">' + badge + '</span>' +
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

function hasCheckedToday() {
  var today = todayISO();
  return state.log.some(function(l) { return l.date === today && l.type === 'ok'; });
}

function isChecked(dateStr) {
  return state.log.some(function(l) { return l.date === dateStr && l.type === 'ok'; });
}

function getWeekDays() {
  var today = new Date();
  var days  = [];
  var dow   = today.getDay();
  for (var i = 0; i <= dow; i++) {
    var d = new Date(today);
    d.setDate(d.getDate() - (dow - i));
    days.push(toISO(d));
  }
  return days;
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
    if (data.pending_checkin && !hasCheckedToday()) doCheckin();
    saveState();
  })
  .catch(function() {});
}

/* ── ARRANQUE ─────────────────────────────────────────── */
loadState();
nav('home');
setInterval(pollTotem, 30000);