#ifndef WEBPAGE_ADMIN_H
#define WEBPAGE_ADMIN_H
#include <string>

inline std::string getAdminTemplate() {
    // Spezzato in chunk per rispettare il limite MSVC C2026 (max 16380 char per literal)
    std::string html;

    // ── CHUNK 1 : head + stili ─────────────────────────────────────────────
    html += R"raw(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>OCU // ACCESS PANEL</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;700&family=Inter:wght@400;600;800&display=swap" rel="stylesheet">
<style>
/* ── RESET ── */
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}

/* ── TOKENS ── */
:root{
  --bg:#06070d;
  --surface:#0d0f1a;
  --surface2:#121420;
  --border:rgba(255,255,255,0.06);
  --border-hi:rgba(99,102,241,0.5);
  --indigo:#6366f1;
  --indigo-dim:rgba(99,102,241,0.15);
  --indigo-glow:rgba(99,102,241,0.08);
  --green:#22d3a8;
  --red:#f43f5e;
  --amber:#fbbf24;
  --text:#e2e8f0;
  --text-dim:#64748b;
  --text-faint:rgba(255,255,255,0.18);
  --mono:'IBM Plex Mono',monospace;
  --sans:'Inter',sans-serif;
  --radius:14px;
  --radius-sm:8px;
  --radius-pill:999px;
}

/* ── BASE ── */
body{
  background:var(--bg);
  color:var(--text);
  font-family:var(--sans);
  min-height:100vh;
  /* subtle dot grid */
  background-image:
    radial-gradient(circle at 1px 1px,rgba(99,102,241,0.07) 1px,transparent 0);
  background-size:28px 28px;
}

/* ── LAYOUT ── */
.panel{max-width:860px;margin:0 auto;padding:32px 16px 64px}
@media(min-width:640px){.panel{padding:48px 24px 80px}}

/* ── CARD ── */
.card{
  background:var(--surface);
  border:1px solid var(--border);
  border-radius:var(--radius);
  padding:24px;
  transition:border-color .2s,box-shadow .2s;
}
.card:hover{border-color:var(--border-hi);box-shadow:0 0 28px var(--indigo-glow)}
.card+.card{margin-top:16px}

/* ── SECTION LABEL ── */
.sec-num{font-family:var(--mono);font-weight:700;font-size:15px;color:var(--indigo)}
.sec-title{font-family:var(--mono);text-transform:uppercase;letter-spacing:.12em;font-size:11px;color:var(--text)}

/* ── INPUTS ── */
input[type=text],input[type=password],input[type=datetime-local],input[type=time]{
  width:100%;background:rgba(0,0,0,0.5);
  border:1px solid var(--border);
  border-radius:var(--radius-sm);
  color:var(--indigo);
  font-family:var(--mono);font-size:11px;
  padding:12px 14px;outline:none;
  transition:border-color .15s;
  letter-spacing:.04em;
}
input[type=text]:focus,
input[type=password]:focus,
input[type=datetime-local]:focus,
input[type=time]:focus{border-color:var(--indigo)}
input[type=datetime-local]::-webkit-calendar-picker-indicator{
  filter:invert(.5) sepia(1) saturate(5) hue-rotate(200deg);cursor:pointer
}
input::placeholder{color:var(--text-dim)}

/* ── BUTTONS ── */
.btn{
  display:inline-flex;align-items:center;justify-content:center;gap:6px;
  font-family:var(--mono);font-size:10px;font-weight:700;
  text-transform:uppercase;letter-spacing:.12em;
  border-radius:var(--radius-sm);border:none;cursor:pointer;
  padding:12px 20px;transition:all .15s;
}
.btn-primary{background:var(--indigo);color:#fff}
.btn-primary:hover{background:#5254cc;box-shadow:0 0 16px rgba(99,102,241,.4)}
.btn-ghost{background:rgba(255,255,255,.04);color:var(--text-dim);border:1px solid var(--border)}
.btn-ghost:hover{border-color:var(--border-hi);color:var(--text)}
.btn-danger{background:rgba(244,63,94,.08);color:var(--red);border:1px solid rgba(244,63,94,.3)}
.btn-danger:hover{background:var(--red);color:#fff;border-color:var(--red)}
.btn-icon{padding:8px;border-radius:var(--radius-sm);background:var(--indigo-dim);border:1px solid rgba(99,102,241,.2);color:var(--indigo);cursor:pointer;transition:all .15s}
.btn-icon:hover{background:var(--indigo);color:#fff}
.btn-full{width:100%}

/* ── TOGGLE BUTTON ── */
.toggle-btn{
  display:flex;align-items:center;gap:8px;
  background:none;border:none;cursor:pointer;
  font-family:var(--mono);font-size:10px;
  color:rgba(99,102,241,.5);letter-spacing:.1em;text-transform:uppercase;
  transition:color .15s;
}
.toggle-btn:hover{color:var(--indigo)}
.toggle-arrow{width:10px;height:10px;transition:transform .2s;flex-shrink:0}

/* ── SELECT ── */
select{
  width:100%;background:rgba(0,0,0,.5);
  border:1px solid var(--border);border-radius:var(--radius-sm);
  color:var(--indigo);font-family:var(--mono);font-size:11px;
  padding:6px 10px;outline:none;cursor:pointer;
  scrollbar-width:thin;scrollbar-color:var(--indigo) transparent;
}
select option{background:#0d0f1a;padding:6px}
select option:checked{background:var(--indigo)}

/* ── SCROLLBAR ── */
.scroll-thin::-webkit-scrollbar{width:3px}
.scroll-thin::-webkit-scrollbar-thumb{background:var(--indigo);border-radius:99px}

/* ── BADGES ── */
.badge{
  display:inline-flex;align-items:center;
  font-family:var(--mono);font-size:9px;
  padding:2px 8px;border-radius:var(--radius-pill);
  border:1px solid;white-space:nowrap;
}
.badge-blue{color:rgba(99,102,241,.9);border-color:rgba(99,102,241,.3);background:rgba(99,102,241,.08)}
.badge-green{color:var(--green);border-color:rgba(34,211,168,.3);background:rgba(34,211,168,.08)}
.badge-red{color:var(--red);border-color:rgba(244,63,94,.3);background:rgba(244,63,94,.08)}
.badge-name{color:#a78bfa;border-color:rgba(167,139,250,.3);background:rgba(167,139,250,.08)}

/* ── ENTRY ROWS ── */
.entry-row{
  display:flex;align-items:center;justify-content:space-between;
  padding:10px 14px;border-bottom:1px solid var(--border);
  transition:background .12s;cursor:default;
}
.entry-row:last-child{border-bottom:none}
.entry-row:hover{background:var(--indigo-glow)}
.entry-addr{font-family:var(--mono);font-size:11px;color:rgba(99,102,241,.8);
  overflow:hidden;text-overflow:ellipsis;white-space:nowrap;max-width:220px}
@media(min-width:640px){.entry-addr{max-width:340px}}
.entry-name{font-family:var(--mono);font-size:10px;color:#a78bfa;margin-top:2px}

/* ── DAY PILLS ── */
.day-pill{
  cursor:pointer;user-select:none;font-family:var(--mono);font-size:9px;
  padding:3px 7px;border-radius:var(--radius-sm);
  border:1px solid rgba(99,102,241,.25);color:rgba(99,102,241,.4);
  background:transparent;transition:all .12s;
}
.day-pill.active{background:rgba(99,102,241,.25);border-color:var(--indigo);color:#a5b4fc}

/* ── SLOT CARD ── */
.slot-card{
  background:rgba(0,0,0,.3);border:1px solid rgba(99,102,241,.12);
  border-radius:var(--radius-sm);padding:10px 12px;position:relative;
}

/* ── DRAWER ── */
#editDrawer{
  position:fixed;top:0;right:-440px;height:100vh;width:420px;
  background:rgba(9,10,18,.98);backdrop-filter:blur(24px);
  border-left:1px solid var(--border-hi);
  z-index:80;transition:right .28s cubic-bezier(.4,0,.2,1);
  overflow-y:auto;padding:28px 24px;
}
#editDrawer.open{right:0}
#drawerOverlay{
  display:none;position:fixed;inset:0;
  background:rgba(0,0,0,.55);z-index:79;backdrop-filter:blur(2px);
}
#drawerOverlay.open{display:block}
@media(max-width:480px){#editDrawer{width:100vw;right:-100vw}}

/* ── STATUS BAR ── */
.status-bar{
  display:flex;align-items:center;justify-content:space-between;
  background:var(--surface);border:1px solid var(--border);
  border-radius:var(--radius);padding:14px 20px;
}

/* ── WARNING ── */
.warning{
  background:linear-gradient(90deg,rgba(251,191,36,.08),rgba(251,191,36,.04));
  border:1px solid rgba(251,191,36,.25);border-radius:var(--radius);
  padding:14px 18px;display:flex;align-items:center;justify-content:space-between;gap:12px;
}

/* ── MODALS ── */
.modal-backdrop{
  display:none;position:fixed;inset:0;
  background:rgba(0,0,0,.85);backdrop-filter:blur(6px);
  z-index:90;align-items:center;justify-content:center;padding:16px;
}
.modal-backdrop.open{display:flex}
.modal{
  background:var(--surface2);border:1px solid var(--border);
  border-radius:var(--radius);padding:28px;width:100%;max-width:360px;
  border-top:3px solid var(--indigo);
}
.modal-danger{border-top-color:var(--red)}

/* ── COLLAPSIBLE ── */
.collapsible{overflow:hidden;transition:max-height .25s ease,opacity .2s}
.collapsible.collapsed{max-height:0!important;opacity:0}

/* ── DEVICE NAME CHIP ── */
.device-chip{
  display:inline-flex;align-items:center;gap:8px;
  background:var(--indigo-dim);border:1px solid rgba(99,102,241,.3);
  border-radius:var(--radius-pill);padding:4px 12px 4px 8px;
  font-family:var(--mono);font-size:11px;color:var(--indigo);
  cursor:pointer;transition:all .15s;
}
.device-chip:hover{background:rgba(99,102,241,.25)}
.device-dot{width:6px;height:6px;border-radius:50%;background:var(--green);flex-shrink:0}

/* ── PING ── */
@keyframes ping{0%{transform:scale(1);opacity:.7}70%{transform:scale(1.8);opacity:0}100%{transform:scale(1.8);opacity:0}}
.ping{animation:ping 1.4s cubic-bezier(0,0,.2,1) infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.pulse{animation:pulse 2s ease-in-out infinite}

/* ── GRID ── */
.grid-2{display:grid;grid-template-columns:1fr 1fr;gap:12px}
@media(max-width:480px){.grid-2{grid-template-columns:1fr}}
.flex{display:flex}.flex-col{flex-direction:column}
.items-center{align-items:center}.justify-between{justify-content:space-between}
.gap-2{gap:8px}.gap-3{gap:12px}.gap-4{gap:16px}
.mt-1{margin-top:4px}.mt-2{margin-top:8px}.mt-3{margin-top:12px}.mt-4{margin-top:16px}
.mb-2{margin-bottom:8px}.mb-3{margin-bottom:12px}.mb-4{margin-bottom:16px}.mb-6{margin-bottom:24px}
.space-y-2>*+*{margin-top:8px}.space-y-3>*+*{margin-top:12px}.space-y-4>*+*{margin-top:16px}
.text-sm{font-size:13px}.text-xs{font-size:11px}.text-xxs{font-size:10px}.text-micro{font-size:9px}
.mono{font-family:var(--mono)}.uppercase{text-transform:uppercase}.tracking{letter-spacing:.1em}
.text-dim{color:var(--text-dim)}.text-faint{color:var(--text-faint)}
.text-indigo{color:var(--indigo)}.text-green{color:var(--green)}.text-red{color:var(--red)}
.text-violet{color:#a78bfa}.text-amber{color:var(--amber)}
.font-bold{font-weight:700}.font-semibold{font-weight:600}
.truncate{overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.break-all{word-break:break-all}
.rounded{border-radius:var(--radius-sm)}.rounded-full{border-radius:var(--radius-pill)}
.w-full{width:100%}.hidden{display:none!important}
.flex-1{flex:1}.shrink-0{flex-shrink:0}
.ml-1{margin-left:4px}.ml-2{margin-left:8px}.mr-2{margin-right:8px}
.p-1{padding:4px}.p-2{padding:8px}
</style>
</head>
<body>
<div class="panel">
)raw";

    // ── CHUNK 2 : HTML struttura ────────────────────────────────────────────
    html += R"raw(
<!-- PIN WARNING -->
<div id="pinWarning" class="hidden warning mb-4">
  <div class="flex items-center gap-2">
    <svg width="16" height="16" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" class="text-amber shrink-0"><path stroke-linecap="round" stroke-linejoin="round" d="M12 9v2m0 4h.01M10.29 3.86L1.82 18a2 2 0 001.71 3h16.94a2 2 0 001.71-3L13.71 3.86a2 2 0 00-3.42 0z"/></svg>
    <span class="mono text-micro uppercase tracking text-amber font-bold">Security Alert: Default PIN active. Update required.</span>
  </div>
  <button onclick="openPinModal()" class="btn btn-ghost text-xxs shrink-0" style="padding:6px 12px">Reconfigure</button>
</div>

<!-- HEADER -->
<div class="mb-6">
  <div class="mb-4">
    <h1 style="font-family:var(--mono);font-size:clamp(18px,4vw,28px);font-weight:800;letter-spacing:-.02em;color:#fff">
      OCU<span style="color:var(--indigo)"> //</span> ACCESS PANEL
    </h1>
    <p class="text-micro mono text-dim uppercase tracking mt-1">On-Chain Unlock &mdash; Protocol Gate Control</p>
  </div>

  <!-- STATUS BAR -->
  <div class="status-bar">
    <div>
      <!-- Device name chip — cliccabile per rinominare -->
      <div id="deviceChip" class="device-chip" onclick="openDeviceNameModal()" title="Click to rename device">
        <span class="device-dot"></span>
        <span id="deviceNameLabel" class="truncate" style="max-width:160px">LOADING...</span>
        <svg width="10" height="10" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M15.232 5.232l3.536 3.536M9 11l6-6 3 3-6 6H9v-3z"/></svg>
      </div>
      <span id="lastSync" class="text-micro mono text-faint uppercase tracking mt-1" style="display:block">SYNC: --:--:--</span>
    </div>
    <div class="flex items-center gap-2">
      <button onclick="openPinModal()" class="btn-icon" title="Update PIN">
        <svg width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M9 12l2 2 4-4m5.618-4.016A11.955 11.955 0 0112 2.944a11.955 11.955 0 01-8.618 3.04A12.02 12.02 0 003 9c0 5.591 3.824 10.29 9 11.622 5.176-1.332 9-6.03 9-11.622 0-1.042-.133-2.052-.382-3.016z"/></svg>
      </button>
      <button onclick="terminateSession()" class="btn btn-danger text-xxs" style="padding:8px 14px">
        <svg width="12" height="12" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M17 16l4-4m0 0l-4-4m4 4H7m6 4v1a3 3 0 01-3 3H6a3 3 0 01-3-3V7a3 3 0 013-3h4a3 3 0 013 3v1"/></svg>
        Exit
      </button>
      <div style="position:relative;width:28px;height:28px">
        <div style="position:absolute;inset:0;border-radius:50%;background:rgba(99,102,241,.2)" class="ping"></div>
        <div style="position:absolute;inset:4px;border-radius:50%;background:var(--indigo)"></div>
      </div>
    </div>
  </div>
</div>

<!-- ── 01 MANUAL INJECTION ── -->
<div class="card mb-4">
  <div class="flex items-center gap-2 mb-4">
    <span class="sec-num">01 //</span>
    <span class="sec-title">Manual Injection</span>
  </div>

  <!-- Address + Name row -->
  <div class="grid-2 mb-3">
    <input type="text" id="inj_addr" placeholder="ef… Enjin Matrixchain address" maxlength="64">
    <input type="text" id="inj_name" placeholder="Display name (max 20 chars)" maxlength="20">
  </div>

  <!-- Temporal toggle -->
  <div class="mb-3">
    <button class="toggle-btn" onclick="toggleSection('temporalSection','temporalArrow')">
      <svg id="temporalArrow" class="toggle-arrow" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M9 5l7 7-7 7"/></svg>
      Temporal constraints <span class="text-faint ml-1">(optional)</span>
    </button>
  </div>

  <div id="temporalSection" class="collapsible collapsed" style="max-height:600px">
    <div style="background:rgba(0,0,0,.25);border:1px solid rgba(99,102,241,.1);border-radius:var(--radius-sm);padding:16px;margin-bottom:12px">
      <div class="grid-2 mb-3">
        <div>
          <label class="text-micro mono text-dim uppercase tracking mb-1" style="display:block">Valid From <span class="text-faint">(empty = no limit)</span></label>
          <input type="datetime-local" id="inj_valid_from">
        </div>
        <div>
          <label class="text-micro mono text-dim uppercase tracking mb-1" style="display:block">Valid To <span class="text-faint">(empty = no limit)</span></label>
          <input type="datetime-local" id="inj_valid_to">
        </div>
      </div>
      <div>
        <div class="flex items-center justify-between mb-2">
          <label class="text-micro mono text-dim uppercase tracking">Recurring Schedule <span class="text-faint">(empty = always)</span></label>
          <button class="btn btn-ghost text-micro" style="padding:4px 10px" onclick="addScheduleSlot('inj')">+ Add Slot</button>
        </div>
        <div id="inj_scheduleSlots" class="space-y-2"></div>
      </div>
    </div>
  </div>

  <button class="btn btn-primary btn-full" onclick="authorizeManual()">Inject Identity</button>
</div>
)raw";

    // ── CHUNK 3 : sezioni 02 03 footer + drawer ────────────────────────────
    html += R"raw(
<!-- ── 02 INTERCEPTION LOG ── -->
<div class="card mb-4">
  <div class="flex items-center justify-between mb-4">
    <div class="flex items-center gap-2">
      <span class="sec-num">02 //</span>
      <span class="sec-title">Interception Log</span>
    </div>
    <button class="btn btn-ghost text-micro" style="padding:6px 12px" onclick="refreshAll()">
      <svg width="12" height="12" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" id="refreshIcon" style="transition:transform .5s"><path stroke-linecap="round" stroke-linejoin="round" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"/></svg>
      Rescan
    </button>
  </div>
  <select id="pendingBox" size="5" class="scroll-thin mb-2"></select>
  <input type="text" id="pendingName" placeholder="Assign a name (optional, max 20 chars)" maxlength="20" style="margin-bottom:8px">
  <button class="btn btn-ghost btn-full" onclick="linkPending()">Link Selected Identity</button>
</div>

<!-- ── 03 AUTHORIZED ENTITIES ── -->
<div class="card mb-6" style="border-color:rgba(244,63,94,.12)">
  <div class="flex items-center justify-between mb-4">
    <div class="flex items-center gap-2">
      <span class="sec-num" style="color:var(--red)">03 //</span>
      <span class="sec-title">Authorized Entities</span>
    </div>
    <span class="text-micro mono text-faint uppercase tracking">Click to edit</span>
  </div>

  <div id="activeList" class="scroll-thin mb-3"
       style="border:1px solid var(--border);border-radius:var(--radius-sm);min-height:100px;max-height:280px;overflow-y:auto">
    <div id="activeListEmpty" style="display:flex;align-items:center;justify-content:center;height:80px">
      <span class="mono text-micro text-faint">// NO_NODE_DETECTED</span>
    </div>
  </div>

  <button class="btn btn-danger btn-full" onclick="revokeSelected()">Revoke Access Protocol</button>
</div>

<!-- ── 04 ROUTE CONTROL ── -->
<div class="card mb-4">
  <div class="flex items-center justify-between mb-4">
    <div class="flex items-center gap-2">
      <span class="sec-num" style="color:#22d3a8">04 //</span>
      <span class="sec-title">Route Control</span>
    </div>
    <button class="btn btn-ghost text-micro" style="padding:6px 12px" onclick="refreshRoutes()">
      <svg width="12" height="12" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24" id="routeRefreshIcon" style="transition:transform .5s"><path stroke-linecap="round" stroke-linejoin="round" d="M4 4v5h.582m15.356 2A8.001 8.001 0 004.582 9m0 0H9m11 11v-5h-.581m0 0a8.003 8.003 0 01-15.357-2m15.357 2H15"/></svg>
      Refresh
    </button>
  </div>
  <div id="routeList" style="border:1px solid var(--border);border-radius:var(--radius-sm);min-height:80px;overflow:hidden">
    <div id="routeListEmpty" style="display:flex;align-items:center;justify-content:center;height:60px">
      <span class="mono text-micro text-faint">// LOADING...</span>
    </div>
  </div>
</div>

<!-- ── 05 ACCESS LOG ── -->
<div class="card mb-6">
  <div class="flex items-center justify-between mb-4">
    <div class="flex items-center gap-2">
      <span class="sec-num" style="color:#f59e0b">05 //</span>
      <span class="sec-title">Access Log</span>
    </div>
    <div class="flex gap-2">
      <button class="btn btn-ghost text-micro" style="padding:6px 12px" onclick="loadLogs()">View Last 50</button>
      <button class="btn btn-ghost text-micro" style="padding:6px 12px" onclick="downloadLogs()">
        <svg width="12" height="12" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M4 16v1a3 3 0 003 3h10a3 3 0 003-3v-1m-4-4l-4 4m0 0l-4-4m4 4V4"/></svg>
        Download
      </button>
    </div>
  </div>
  <div id="logBox" class="scroll-thin" style="background:rgba(0,0,0,.4);border:1px solid var(--border);border-radius:var(--radius-sm);min-height:80px;max-height:260px;overflow-y:auto;padding:12px;display:none">
  </div>
  <div id="logEmpty" style="background:rgba(0,0,0,.4);border:1px solid var(--border);border-radius:var(--radius-sm);height:60px;display:flex;align-items:center;justify-content:center">
    <span class="mono text-micro text-faint">// Click &quot;View Last 50&quot; to load</span>
  </div>
</div>

<!-- MODAL: OPEN DOOR -->
<!-- MODAL: OPEN -->
<div id="openModal" class="modal-backdrop">
  <div class="modal">
    <h3 class="mono font-bold text-sm mb-1" style="color:var(--green)">Open Route</h3>
    <p id="openModalPidLabel" class="mono text-micro text-dim mb-4"></p>
    <div class="space-y-3">
      <div>
        <label class="text-micro mono text-dim uppercase tracking mb-1" style="display:block">Auto-revert at <span class="text-faint">(leave empty = manual only)</span></label>
        <input type="datetime-local" id="openModalUntil">
      </div>
      <button class="btn btn-full" onclick="executeRouteMode(null,1,'open')" style="background:#22d3a8;color:#000;font-family:var(--mono);font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:.12em;border-radius:var(--radius-sm);border:none;padding:12px;cursor:pointer">Open to Everyone</button>
      <button class="btn btn-ghost btn-full" onclick="closeRouteModeModal('open')">Cancel</button>
    </div>
  </div>
</div>

<!-- MODAL: LOCK -->
<div id="lockModal" class="modal-backdrop">
  <div class="modal">
    <h3 class="mono font-bold text-sm mb-1" style="color:var(--red)">Lock Route</h3>
    <p id="lockModalPidLabel" class="mono text-micro text-dim mb-4"></p>
    <div class="space-y-3">
      <div>
        <label class="text-micro mono text-dim uppercase tracking mb-1" style="display:block">Auto-revert at <span class="text-faint">(leave empty = manual only)</span></label>
        <input type="datetime-local" id="lockModalUntil">
      </div>
      <button class="btn btn-full" onclick="executeRouteMode(null,2,'lock')" style="background:rgba(244,63,94,.15);color:var(--red);border:1px solid rgba(244,63,94,.4);font-family:var(--mono);font-size:10px;font-weight:700;text-transform:uppercase;letter-spacing:.12em;border-radius:var(--radius-sm);padding:12px;cursor:pointer">Lock — Admin Only</button>
      <button class="btn btn-ghost btn-full" onclick="closeRouteModeModal('lock')">Cancel</button>
    </div>
  </div>
</div>


<div style="border:1px solid var(--border);border-radius:var(--radius);padding:16px 20px;
     background:rgba(255,255,255,.01);display:flex;align-items:center;justify-content:space-between">
  <span class="mono text-micro text-faint uppercase tracking"><span class="text-indigo">&gt;</span> Session: Admin</span>
  <div class="flex items-center gap-3">
    <button onclick="checkNftInfo()" class="btn btn-ghost text-micro" style="padding:5px 12px;font-size:9px">
      NFT Info
    </button>
    <div class="flex gap-2">
      <div style="width:6px;height:6px;border-radius:50%;background:rgba(99,102,241,.2)"></div>
      <div style="width:6px;height:6px;border-radius:50%;background:var(--indigo)" class="pulse"></div>
    </div>
  </div>
</div>

</div><!-- /.panel -->

<!-- ================================================================
     DRAWER — edit entry (name + temporal constraints)
     ================================================================ -->
<div id="drawerOverlay" onclick="closeDrawer()"></div>
<div id="editDrawer">
  <div class="flex items-center justify-between mb-4">
    <div>
      <h3 class="mono font-bold text-sm" style="color:#fff">Identity Config</h3>
      <p id="drawerAddr" class="mono text-micro text-dim mt-1 break-all"></p>
    </div>
    <button onclick="closeDrawer()" class="btn-icon" style="flex-shrink:0">
      <svg width="14" height="14" fill="none" stroke="currentColor" stroke-width="2" viewBox="0 0 24 24"><path stroke-linecap="round" stroke-linejoin="round" d="M6 18L18 6M6 6l12 12"/></svg>
    </button>
  </div>

  <!-- Name field -->
  <div class="mb-4">
    <label class="text-micro mono text-dim uppercase tracking mb-2" style="display:block">Display Name <span class="text-faint">(max 20 chars)</span></label>
    <input type="text" id="drw_name" placeholder="e.g. Mario Rossi" maxlength="20">
  </div>

  <!-- Access window -->
  <div class="mb-4">
    <p class="text-micro mono text-dim uppercase tracking mb-2">Access Window</p>
    <div class="space-y-2">
      <div>
        <label class="text-micro mono text-faint uppercase mb-1" style="display:block">Valid From</label>
        <input type="datetime-local" id="drw_valid_from">
      </div>
      <div>
        <label class="text-micro mono text-faint uppercase mb-1" style="display:block">Valid To</label>
        <input type="datetime-local" id="drw_valid_to">
      </div>
    </div>
  </div>

  <!-- Recurring schedule -->
  <div class="mb-4">
    <div class="flex items-center justify-between mb-2">
      <p class="text-micro mono text-dim uppercase tracking">Recurring Schedule</p>
      <button class="btn btn-ghost text-micro" style="padding:4px 10px" onclick="addScheduleSlot('drw')">+ Add Slot</button>
    </div>
    <div id="drw_scheduleSlots" class="space-y-2"></div>
  </div>

  <div class="space-y-2">
    <button class="btn btn-primary btn-full" onclick="saveDrawer()">Commit</button>
    <button class="btn btn-ghost btn-full" onclick="clearDrawerConstraints()">Clear Constraints</button>
  </div>
</div>
)raw";

    // ── CHUNK 4 : modali ───────────────────────────────────────────────────
    html += R"raw(
<!-- ── MODAL: PIN ── -->
<div id="pinModal" class="modal-backdrop">
  <div class="modal">
    <h3 class="mono font-bold text-sm mb-1" style="color:#fff">Update Security Protocol</h3>
    <p class="text-micro mono text-dim mb-4">SHA256 + SN salt</p>
    <div class="space-y-3">
      <input type="password" id="newPin" placeholder="NEW PIN (4-8 DIGITS)" maxlength="8" style="text-align:center;letter-spacing:.5em">
      <input type="password" id="confirmPin" placeholder="CONFIRM PIN" maxlength="8" style="text-align:center;letter-spacing:.5em">
      <button class="btn btn-primary btn-full mt-2" onclick="executePinUpdate()">Sign &amp; Commit</button>
      <button class="btn btn-ghost btn-full" onclick="closePinModal()">Abort</button>
    </div>
  </div>
</div>

<!-- ── MODAL: DEVICE NAME ── -->
<div id="deviceNameModal" class="modal-backdrop">
  <div class="modal">
    <h3 class="mono font-bold text-sm mb-1" style="color:#fff">Device Label</h3>
    <p class="text-micro mono text-dim mb-4">Shown in the status bar (max 20 chars)</p>
    <div class="space-y-3">
      <input type="text" id="deviceNameInput" placeholder="e.g. BUILDING-A ENTRANCE" maxlength="20">
      <button class="btn btn-primary btn-full" onclick="saveDeviceName()">Save Label</button>
      <button class="btn btn-ghost btn-full" onclick="closeDeviceNameModal()">Cancel</button>
    </div>
  </div>
</div>

<!-- ── MODAL: CONFIRM ── -->
<div id="confirmModal" class="modal-backdrop">
  <div class="modal modal-danger">
    <div class="flex items-center gap-2 mb-3">
      <span class="mono font-bold text-sm text-red pulse">&gt;_</span>
      <h3 class="mono font-bold text-sm" style="color:#fff">SYSTEM WARNING</h3>
    </div>
    <p id="confirmText" class="mono text-xs text-dim mb-4" style="line-height:1.6;white-space:pre-line"></p>
    <div class="flex gap-3">
      <button class="btn btn-ghost flex-1" onclick="closeConfirm()">Cancel</button>
      <button class="btn btn-danger flex-1" onclick="executeRemove()">Execute</button>
    </div>
  </div>
</div>

<!-- ── MODAL: ALERT ── -->
<div id="alertModal" class="modal-backdrop">
  <div id="alertBox" class="modal">
    <div class="flex items-center gap-2 mb-3">
      <span id="alertIcon" class="mono font-bold text-sm">&gt;_</span>
      <h3 id="alertTitle" class="mono font-bold text-sm" style="color:#fff"></h3>
    </div>
    <p id="alertText" class="mono text-xs text-dim mb-4 break-all" style="line-height:1.6"></p>
    <button class="btn btn-ghost btn-full" onclick="closeAlert()">Acknowledge</button>
  </div>
</div>
)raw";

    // ── CHUNK 5 : JS parte 1 (state + utilities + schedule) ───────────────
    std::string js;
    js += R"raw(
<script>
// ── STATE ──────────────────────────────────────────────────────────────────
let isTerminating = false;
let pendingRemoveAddr = "";
let selectedActiveAddr = "";
let activeEntriesMap = {};   // addr → entry object

const DAY_NAMES = ["Sun","Mon","Tue","Wed","Thu","Fri","Sat"];
const DEVICE_NAME_KEY = "ocu_device_name";

// ── UTILITIES ──────────────────────────────────────────────────────────────
function toDatetimeLocal(unix) {
    if (!unix) return "";
    const d = new Date(unix * 1000);
    const p = n => String(n).padStart(2,"0");
    return `${d.getFullYear()}-${p(d.getMonth()+1)}-${p(d.getDate())}T${p(d.getHours())}:${p(d.getMinutes())}`;
}
function fromDatetimeLocal(s) {
    if (!s) return 0;
    return Math.floor(new Date(s).getTime() / 1000);
}
function shortDate(unix) {
    if (!unix) return null;
    return new Date(unix * 1000).toLocaleDateString(undefined, {day:"2-digit",month:"short",year:"2-digit"});
}
function isExpired(entry) {
    return entry.valid_to && entry.valid_to < Math.floor(Date.now()/1000);
}

// ── COLLAPSIBLE ────────────────────────────────────────────────────────────
function toggleSection(id, arrowId) {
    const el = document.getElementById(id);
    const arrow = document.getElementById(arrowId);
    const collapsed = el.classList.toggle("collapsed");
    if (arrow) arrow.style.transform = collapsed ? "" : "rotate(90deg)";
}

// ── DEVICE NAME ────────────────────────────────────────────────────────────
function loadDeviceName() {
    const cached = localStorage.getItem(DEVICE_NAME_KEY) || "";
    if (cached) document.getElementById("deviceNameLabel").textContent = cached;
    apiFetch("/api/admin/device/label").then(r => {
        if (r && r.ok) r.json().then(d => {
            const serverLabel = d.label || "";
            if (serverLabel) {
                // Server has a value — use it as source of truth
                localStorage.setItem(DEVICE_NAME_KEY, serverLabel);
                document.getElementById("deviceNameLabel").textContent = serverLabel;
            } else if (cached) {
                // Server is empty but localStorage has a value — push it to server
                apiFetch("/api/admin/device/label", "POST", { label: cached });
                document.getElementById("deviceNameLabel").textContent = cached;
            } else {
                document.getElementById("deviceNameLabel").textContent = "OCU DEVICE";
            }
        });
    });
}
function saveDeviceName() {
    const val = document.getElementById("deviceNameInput").value.trim().substring(0,20);
    if (val) {
        localStorage.setItem(DEVICE_NAME_KEY, val);
        document.getElementById("deviceNameLabel").textContent = val;
        // Persiste sul server
        apiFetch("/api/admin/device/label", "POST", { label: val });
    }
    closeDeviceNameModal();
}

// ── SCHEDULE SLOTS ─────────────────────────────────────────────────────────
function addScheduleSlot(prefix, existing) {
    const container = document.getElementById(prefix + "_scheduleSlots");
    const idx = container.children.length;
    const id  = prefix + "_slot_" + idx;
    const activeDays = (existing && existing.days) ? existing.days : [];
    const fromVal = (existing && existing.from) ? existing.from : "";
    const toVal   = (existing && existing.to)   ? existing.to   : "";

    const daysHtml = DAY_NAMES.map((n,i) =>
        `<span class="day-pill ${activeDays.includes(i)?"active":""}" data-day="${i}" onclick="toggleDay(this)">${n}</span>`
    ).join("");

    const card = document.createElement("div");
    card.className = "slot-card";
    card.id = id;
    card.innerHTML = `
        <button onclick="removeSlot('${id}')" style="position:absolute;top:8px;right:8px;background:none;border:none;cursor:pointer;font-family:var(--mono);font-size:10px;color:rgba(255,255,255,.2)" onmouseover="this.style.color='var(--red)'" onmouseout="this.style.color='rgba(255,255,255,.2)'">✕</button>
        <div class="mb-2">
            <p class="text-micro mono text-faint uppercase tracking mb-2">Days <span style="opacity:.5">(empty = every day)</span></p>
            <div style="display:flex;flex-wrap:wrap;gap:4px">${daysHtml}</div>
        </div>
        <div class="grid-2">
            <div>
                <label class="text-micro mono text-faint uppercase" style="display:block;margin-bottom:4px">From</label>
                <input type="time" value="${fromVal}" class="slot-from">
            </div>
            <div>
                <label class="text-micro mono text-faint uppercase" style="display:block;margin-bottom:4px">To</label>
                <input type="time" value="${toVal}" class="slot-to">
            </div>
        </div>`;
    container.appendChild(card);
}
function toggleDay(el) { el.classList.toggle("active"); }
function removeSlot(id) { const el = document.getElementById(id); if (el) el.remove(); }
function readScheduleSlots(prefix) {
    const slots = [];
    for (const card of document.getElementById(prefix+"_scheduleSlots").children) {
        const days = [];
        card.querySelectorAll(".day-pill.active").forEach(p => days.push(parseInt(p.dataset.day)));
        slots.push({ days, from: card.querySelector(".slot-from").value, to: card.querySelector(".slot-to").value });
    }
    return slots;
}
function clearScheduleSlots(prefix) {
    document.getElementById(prefix+"_scheduleSlots").innerHTML = "";
}
)raw";

    // ── CHUNK 6 : JS parte 2 (drawer + render + api) ──────────────────────
    js += R"raw(
// ── DRAWER ─────────────────────────────────────────────────────────────────
function openDrawer(addr) {
    selectedActiveAddr = addr;
    const e = activeEntriesMap[addr] || {};
    document.getElementById("drawerAddr").textContent = addr;
    document.getElementById("drw_name").value         = e.name        || "";
    document.getElementById("drw_valid_from").value   = toDatetimeLocal(e.valid_from || 0);
    document.getElementById("drw_valid_to").value     = toDatetimeLocal(e.valid_to   || 0);
    clearScheduleSlots("drw");
    (e.schedule || []).forEach(s => addScheduleSlot("drw", s));
    document.getElementById("editDrawer").classList.add("open");
    document.getElementById("drawerOverlay").classList.add("open");
}
function closeDrawer() {
    document.getElementById("editDrawer").classList.remove("open");
    document.getElementById("drawerOverlay").classList.remove("open");
    selectedActiveAddr = "";
}
async function saveDrawer() {
    if (!selectedActiveAddr) return;
    const name       = document.getElementById("drw_name").value.trim().substring(0,20);
    const valid_from = fromDatetimeLocal(document.getElementById("drw_valid_from").value);
    const valid_to   = fromDatetimeLocal(document.getElementById("drw_valid_to").value);
    const schedule   = readScheduleSlots("drw");
    const resp = await apiFetch("/api/admin/whitelist/add","POST",{
        address: selectedActiveAddr,
        name,
        valid_from,
        valid_to,
        schedule: schedule.length > 0 ? schedule : []
    });
    if (resp && resp.ok) {
        // Aggiorna subito la cache locale così openDrawer trova il dato corretto
        if (activeEntriesMap[selectedActiveAddr])
            activeEntriesMap[selectedActiveAddr].name = name;
        closeDrawer();
        refreshAll();
        showAlert("COMMITTED","Saved.","success");
    } else showAlert("FAILED","Could not save.");
}
async function clearDrawerConstraints() {
    if (!selectedActiveAddr) return;
    const name = document.getElementById("drw_name").value.trim().substring(0,20);
    const resp = await apiFetch("/api/admin/whitelist/add","POST",{
        address: selectedActiveAddr, name,
        valid_from:0, valid_to:0, schedule:[]
    });
    if (resp && resp.ok) { closeDrawer(); refreshAll(); showAlert("CLEARED","Constraints removed.","success"); }
}

// ── RENDER ACTIVE LIST ─────────────────────────────────────────────────────
function renderActiveList(entries) {
    const container = document.getElementById("activeList");
    const empty     = document.getElementById("activeListEmpty");
    Array.from(container.children).forEach(c => { if (c.id !== "activeListEmpty") c.remove(); });
    activeEntriesMap = {};
    if (!entries || entries.length === 0) { empty.style.display = "flex"; return; }
    empty.style.display = "none";
    const now = Math.floor(Date.now()/1000);

    entries.forEach(raw => {
        let addr, name, valid_from, valid_to, schedule;
        if (typeof raw === "string") {
            addr = raw; name = ""; valid_from = 0; valid_to = 0; schedule = [];
        } else {
            addr       = raw.address  || "";
            name       = raw.name     || "";
            valid_from = raw.valid_from || 0;
            valid_to   = raw.valid_to   || 0;
            schedule   = raw.schedule   || [];
        }
        if (!addr) return;
        activeEntriesMap[addr] = { address:addr, name, valid_from, valid_to, schedule };

        const expired = valid_to && valid_to < now;
        let badges = "";
        if (valid_from || valid_to) {
            const fs = valid_from ? shortDate(valid_from) : "∞";
            const ts = valid_to   ? shortDate(valid_to)   : "∞";
            badges += `<span class="badge ${expired?"badge-red":"badge-blue"} ml-1">${fs} → ${ts}</span>`;
        }
        if (schedule && schedule.length > 0)
            badges += `<span class="badge badge-green ml-1">&#x23f1; ${schedule.length}sl</span>`;
        if (expired)
            badges += `<span class="badge badge-red ml-1">EXPIRED</span>`;

        const shortAddr = addr.length > 16
            ? addr.substring(0,8) + "…" + addr.substring(addr.length - 6)
            : addr;
        const displayLine = name
            ? `<span style="color:#a78bfa;font-weight:600">${name}</span><span class="text-faint ml-1" style="font-size:9px">${shortAddr}</span>`
            : `&gt; ${addr}`;

        const row = document.createElement("div");
        row.className = "entry-row";
        const isSelected = selectedActiveAddr === addr;
        if (isSelected) row.style.background = "var(--indigo-glow)";
        row.style.cursor = "pointer";
        row.onclick = () => {
            // Deseleziona se già selezionata, altrimenti seleziona
            selectedActiveAddr = (selectedActiveAddr === addr) ? "" : addr;
            renderActiveList(Object.values(activeEntriesMap));
        };
        row.innerHTML = `
            <div style="min-width:0;flex:1">
                <div class="entry-addr">${displayLine}</div>
                <div style="display:flex;flex-wrap:wrap;gap:2px;margin-top:3px">${badges}</div>
            </div>
            <div style="display:flex;gap:6px;margin-left:10px;flex-shrink:0">
                <button onclick="event.stopPropagation();openDrawer('${addr}')"
                    style="background:none;border:1px solid rgba(99,102,241,.2);border-radius:6px;padding:4px 8px;cursor:pointer;font-family:var(--mono);font-size:9px;color:rgba(99,102,241,.6);transition:all .12s"
                    onmouseover="this.style.color='var(--indigo)';this.style.borderColor='var(--indigo)'"
                    onmouseout="this.style.color='rgba(99,102,241,.6)';this.style.borderColor='rgba(99,102,241,.2)'">Edit</button>
                <button onclick="event.stopPropagation();showConfirm('${addr}')"
                    style="background:none;border:1px solid rgba(244,63,94,.2);border-radius:6px;padding:4px 8px;cursor:pointer;font-family:var(--mono);font-size:9px;color:rgba(244,63,94,.4);transition:all .12s"
                    onmouseover="this.style.color='var(--red)';this.style.borderColor='var(--red)'"
                    onmouseout="this.style.color='rgba(244,63,94,.4)';this.style.borderColor='rgba(244,63,94,.2)'">&#x2715;</button>
            </div>`;
        container.appendChild(row);
    });
}

// ── API FETCH ──────────────────────────────────────────────────────────────
async function apiFetch(url, method = "GET", body = null) {
    const opts = { method, headers:{"Content-Type":"application/json"}, credentials:"include" };
    if (body) opts.body = JSON.stringify(body);
    try {
        const r = await fetch(url, opts);
        if (r.status === 401 || r.status === 403) { redirectToLogin(); return null; }
        return r;
    } catch { return null; }
}
)raw";

    // ── CHUNK 7 : JS parte 3 (actions + init) ─────────────────────────────
    js += R"raw(
// ── ACTIONS ────────────────────────────────────────────────────────────────
function openPinModal()  { document.getElementById("pinModal").classList.add("open"); }
function closePinModal() { document.getElementById("pinModal").classList.remove("open"); }

function openDeviceNameModal() {
    document.getElementById("deviceNameInput").value =
        localStorage.getItem(DEVICE_NAME_KEY) || "";
    document.getElementById("deviceNameModal").classList.add("open");
}
function closeDeviceNameModal() {
    document.getElementById("deviceNameModal").classList.remove("open");
}

async function checkSecurityStatus() {
    const r = await apiFetch("/api/admin/security/status");
    if (r && r.ok) {
        const d = await r.json();
        const w = document.getElementById("pinWarning");
        if (w) w.classList.toggle("hidden", d.is_custom_pin === true);
    }
}

async function executePinUpdate() {
    const p1 = document.getElementById("newPin").value;
    const p2 = document.getElementById("confirmPin").value;
    if (p1 !== p2 || p1.length < 4) { showAlert("INVALID_PIN","PINs must match, min 4 digits."); return; }
    const r = await apiFetch("/api/admin/security/update-pin","POST",{pin:p1});
    if (r && r.ok) {
        closePinModal();
        showAlert("SUCCESS","Security hash updated.","success");
        document.getElementById("newPin").value = "";
        document.getElementById("confirmPin").value = "";
        checkSecurityStatus();
    } else showAlert("FAILED","Could not commit PIN change.");
}

function showAlert(title, text, type = "error") {
    const box  = document.getElementById("alertBox");
    const icon = document.getElementById("alertIcon");
    document.getElementById("alertTitle").innerText = title;
    document.getElementById("alertText").innerText  = text;
    const isErr = type !== "success";
    box.style.borderTopColor = isErr ? "var(--red)" : "var(--indigo)";
    icon.style.color         = isErr ? "var(--red)" : "var(--indigo)";
    document.getElementById("alertModal").classList.add("open");
}
function closeAlert()   { document.getElementById("alertModal").classList.remove("open"); }

function showConfirm(addr) {
    pendingRemoveAddr = addr;
    const name = activeEntriesMap[addr] && activeEntriesMap[addr].name
        ? ` (${activeEntriesMap[addr].name})` : "";
    document.getElementById("confirmText").innerText = `Revoke access for:\n${addr}${name}`;
    document.getElementById("confirmModal").classList.add("open");
}
function closeConfirm() { document.getElementById("confirmModal").classList.remove("open"); }

// ── ROUTE CONTROL ──────────────────────────────────────────────────────────
let openDoorPid = "";

async function refreshRoutes() {
    const icon = document.getElementById("routeRefreshIcon");
    if (icon) icon.style.transform = "rotate(360deg)";
    setTimeout(() => { if (icon) icon.style.transform = ""; }, 500);

    const r = await apiFetch("/api/admin/routes");
    if (!r || !r.ok) return;
    const d = await r.json();
    const routes = d.routes || [];

    const container = document.getElementById("routeList");
    const empty     = document.getElementById("routeListEmpty");
    Array.from(container.children).forEach(c => { if (c.id !== "routeListEmpty") c.remove(); });

    if (routes.length === 0) { empty.style.display = "flex"; return; }
    empty.style.display = "none";

    routes.forEach(rt => {
        const isOpen     = rt.runtime_mode === 1;
        const isLocked   = rt.runtime_mode === 2;
        const canOpen    = rt.allow_open;
        const roleColor  = rt.role === "admin" ? "var(--red)" : rt.role === "none" ? "var(--green)" : "var(--indigo)";
        const roleTxt    = rt.role.toUpperCase();
        const redirectTxt = (rt.redirect && rt.redirect !== "NONE") ? rt.redirect : "—";

        const row = document.createElement("div");
        row.style.cssText = "display:flex;align-items:center;justify-content:space-between;padding:10px 14px;border-bottom:1px solid var(--border)";
        row.innerHTML = `
            <div style="min-width:0;flex:1">
                <div style="display:flex;align-items:center;gap:8px">
                    <span style="font-family:var(--mono);font-size:12px;color:#fff;font-weight:600">/${rt.pid}</span>
                    <span style="font-family:var(--mono);font-size:9px;padding:2px 7px;border-radius:99px;border:1px solid;color:${roleColor};border-color:${roleColor};background:${roleColor}18">${roleTxt}</span>
                    ${isOpen   ? '<span style="font-family:var(--mono);font-size:9px;padding:2px 7px;border-radius:99px;border:1px solid var(--green);color:var(--green);background:rgba(34,211,168,.1)">OPEN</span>' : ""}
                    ${isLocked ? '<span style="font-family:var(--mono);font-size:9px;padding:2px 7px;border-radius:99px;border:1px solid var(--red);color:var(--red);background:rgba(244,63,94,.1)">LOCKED</span>' : ""}
                </div>
                <div style="font-family:var(--mono);font-size:9px;color:var(--text-dim);margin-top:3px">${rt.title} &nbsp;·&nbsp; redirect: ${redirectTxt}</div>
            </div>
            <div style="display:flex;gap:4px;margin-left:12px;flex-shrink:0">
                ${canOpen ? `
                <button onclick="promptOpenDoor('${rt.pid}',${rt.runtime_mode})"
                    style="background:none;border:1px solid ${isOpen ? "var(--red)" : "rgba(34,211,168,.4)"};border-radius:6px;padding:5px 8px;cursor:pointer;font-family:var(--mono);font-size:9px;color:${isOpen ? "var(--red)" : "var(--green)"};transition:all .12s"
                    onmouseover="this.style.opacity='.7'" onmouseout="this.style.opacity='1'">
                    ${isOpen ? "Close" : "Open"}
                </button>
                <button onclick="${isLocked ? `executeRouteMode('${rt.pid}',0,null)` : `promptLockDoor('${rt.pid}',${rt.runtime_mode})`}"
                    style="background:none;border:1px solid ${isLocked ? "var(--amber)" : "rgba(244,63,94,.3)"};border-radius:6px;padding:5px 8px;cursor:pointer;font-family:var(--mono);font-size:9px;color:${isLocked ? "var(--amber)" : "var(--red)"};transition:all .12s"
                    onmouseover="this.style.opacity='.7'" onmouseout="this.style.opacity='1'">
                    ${isLocked ? "Unlock" : "Lock"}
                </button>` : ""}
            </div>`;
        container.appendChild(row);
    });
    const rows = container.querySelectorAll("div[style*='border-bottom']");
    if (rows.length) rows[rows.length-1].style.borderBottom = "none";
}

function promptOpenDoor(pid, currentMode) {
    if (currentMode === 1) {
        executeRouteMode(pid, 0, null);
        return;
    }
    openDoorPid = pid;
    document.getElementById("openModalPidLabel").textContent = "Route: /" + pid;
    document.getElementById("openModalUntil").value = "";
    document.getElementById("openModal").classList.add("open");
}

function promptLockDoor(pid, currentMode) {
    if (currentMode === 2) {
        executeRouteMode(pid, 0, null);
        return;
    }
    openDoorPid = pid;
    document.getElementById("lockModalPidLabel").textContent = "Route: /" + pid;
    document.getElementById("lockModalUntil").value = "";
    document.getElementById("lockModal").classList.add("open");
}

function closeRouteModeModal(type) {
    document.getElementById(type === "open" ? "openModal" : "lockModal").classList.remove("open");
    openDoorPid = "";
}

async function executeRouteMode(pid, mode, modalType) {
    const pidTarget = pid || openDoorPid;
    if (!pidTarget) return;
    let until = 0;
    if (mode !== 0 && modalType) {
        const inputId = modalType === "open" ? "openModalUntil" : "lockModalUntil";
        until = fromDatetimeLocal(document.getElementById(inputId).value);
    }
    if (modalType) closeRouteModeModal(modalType);
    const r = await apiFetch("/api/admin/route/mode","POST",{
        pid: pidTarget, mode, valid_until: until
    });
    if (r && r.ok) {
        const label = mode === 0 ? "RESTORED" : mode === 1 ? "OPENED" : "LOCKED";
        const type  = mode === 2 ? "error" : mode === 1 ? "success" : "error";
        showAlert(`ROUTE_${label}`,
            mode === 0 ? `/${pidTarget} restored to whitelist.` :
            mode === 1 ? `/${pidTarget} is now open to everyone.` :
                         `/${pidTarget} locked — admin only.`, type);
        refreshRoutes();
    } else showAlert("ERROR","Operation failed.");
}

// ── ACCESS LOG ─────────────────────────────────────────────────────────────
let cachedLogLines = [];
)raw";

    // ── CHUNK 8 : JS parte 4 (log + init) ─────────────────────────────
    js += R"raw(
async function loadLogs() {
    const r = await apiFetch("/api/admin/logs?n=50");
    if (!r || !r.ok) return;
    const d = await r.json();
    cachedLogLines = d.lines || [];

    const box   = document.getElementById("logBox");
    const empty = document.getElementById("logEmpty");

    if (cachedLogLines.length === 0) {
        empty.style.display = "flex"; box.style.display = "none"; return;
    }
    empty.style.display = "none";
    box.style.display   = "block";
    box.innerHTML = cachedLogLines.map(line => {
        // Colora SUCCESS verde, REJECTED/DENIED rosso, resto default
        let color = "rgba(255,255,255,.35)";
        if (line.includes("SUCCESS"))               color = "var(--green)";
        else if (line.includes("REJECTED") || line.includes("DENIED") || line.includes("LOCKED")) color = "var(--red)";
        else if (line.includes("ADMIN"))             color = "#a78bfa";
        return `<div style="font-family:var(--mono);font-size:9px;color:${color};line-height:1.8;white-space:nowrap">${line}</div>`;
    }).join("");
    box.scrollTop = box.scrollHeight;
}

function downloadLogs() {
    if (cachedLogLines.length === 0) {
        showAlert("LOG_EMPTY","Load the logs first with 'View Last 50', then download.");
        return;
    }
    const blob = new Blob([cachedLogLines.join("\n")], { type:"text/plain" });
    const a    = document.createElement("a");
    a.href     = URL.createObjectURL(blob);
    a.download = "ocu_access_log_" + new Date().toISOString().slice(0,10) + ".txt";
    a.click();
    URL.revokeObjectURL(a.href);
}

async function checkNftInfo() {
    const r = await apiFetch("/api/admin/nft/info");
    if (!r) return;
    const d = await r.json();
    if (d.status === "eeprom_missing") {
        showAlert("NFT_INFO", "Token ID not configured (EEPROM missing or override=0).");
        return;
    }
    if (d.status === "rpc_error") {
        showAlert("NFT_INFO", "RPC unreachable. Check network and RPC URL.");
        return;
    }
    const minted = d.minted ? "YES" : "NO";
    const owner  = d.owner_address ? `\nOwner: ${d.owner_address}` : "\nOwner: not found";
    showAlert("NFT_INFO",
        `Collection: ${d.collection_id}\nToken ID:   ${d.token_id}\nMinted:     ${minted}${d.minted ? owner : ""}`,
        d.minted ? "success" : "error");
}

async function terminateSession() {
    try { await fetch("/api/logout",{method:"POST",credentials:"include"}); } catch {}
    window.location.href = "/accesses";
}
function redirectToLogin() {
    if (isTerminating) return;
    isTerminating = true;
    window.location.href = "/accesses";
}

async function authorizeManual() {
    const addr = document.getElementById("inj_addr").value.trim();
    const name = document.getElementById("inj_name").value.trim().substring(0,20);
    if (!addr.startsWith("ef") || addr.length < 20) {
        showAlert("FORMAT_ERROR","Address must start with 'ef' (min 20 chars)."); return;
    }
    const temporalOpen = !document.getElementById("temporalSection").classList.contains("collapsed");
    let valid_from = 0, valid_to = 0, schedule = [];
    if (temporalOpen) {
        valid_from = fromDatetimeLocal(document.getElementById("inj_valid_from").value);
        valid_to   = fromDatetimeLocal(document.getElementById("inj_valid_to").value);
        schedule   = readScheduleSlots("inj");
    }
    const r = await apiFetch("/api/admin/whitelist/add","POST",{
        address:addr, name, valid_from, valid_to,
        schedule: schedule.length > 0 ? schedule : []
    });
    if (r && r.ok) {
        document.getElementById("inj_addr").value = "";
        document.getElementById("inj_name").value = "";
        document.getElementById("inj_valid_from").value = "";
        document.getElementById("inj_valid_to").value = "";
        clearScheduleSlots("inj");
        if (!document.getElementById("temporalSection").classList.contains("collapsed"))
            toggleSection("temporalSection","temporalArrow");
        refreshAll();
        showAlert("SUCCESS","Identity linked.","success");
    } else showAlert("INJECTION_FAILED","Already present or invalid format.");
}

async function linkPending() {
    const addr = document.getElementById("pendingBox").value;
    const name = document.getElementById("pendingName").value.trim().substring(0,20);
    if (!addr) return;
    const r = await apiFetch("/api/admin/whitelist/add","POST",{ address:addr, name });
    if (r && r.ok) {
        document.getElementById("pendingName").value = "";
        refreshAll();
    } else {
        const body = r ? await r.json().catch(() => ({status: "unknown"})) : {status: "network_error"};
        showAlert("ERROR", `Operation failed: ${body.status || r?.status}`);
    }
}

function revokeSelected() {
    if (selectedActiveAddr) showConfirm(selectedActiveAddr);
    else showAlert("SELECT_ENTITY","Click an entry in the list first.");
}

async function executeRemove() {
    const addr = pendingRemoveAddr;
    closeConfirm();
    const r = await apiFetch("/api/admin/whitelist/remove","POST",{address:addr});
    if (!r || !r.ok) {
        const body = r ? await r.json().catch(() => ({status: "unknown"})) : {status: "network_error"};
        showAlert("DENIED", `Remove failed: ${body.status || r?.status}`);
    } else {
        if (selectedActiveAddr === addr) selectedActiveAddr = "";
        refreshAll();
    }
}

async function refreshAll() {
    const icon = document.getElementById("refreshIcon");
    if (icon) icon.style.transform = "rotate(360deg)";
    setTimeout(() => { if (icon) icon.style.transform = ""; }, 500);

    const [rP, rA] = await Promise.all([
        apiFetch("/api/admin/whitelist/pending"),
        apiFetch("/api/admin/whitelist/active")
    ]);

    if (rP && rP.ok) {
        const data = await rP.json();
        const pending = data.list || data || [];
        const box = document.getElementById("pendingBox");
        const sel = box.value;
        box.innerHTML = "";
        if (!pending || pending.length === 0) {
            const opt = document.createElement("option");
            opt.text = "// NO_SIGNAL_DETECTED"; opt.disabled = true;
            box.appendChild(opt);
        } else {
            pending.forEach(a => {
                const addr = typeof a === "string" ? a : (a.address || a);
                const opt = document.createElement("option");
                opt.value = addr.trim();
                opt.text  = `> ${addr.trim()}`;
                box.appendChild(opt);
            });
            box.value = sel;
        }
    }

    if (rA && rA.ok) {
        const data = await rA.json();
        renderActiveList(data.list || data || []);
    }

    await refreshRoutes();

    document.getElementById("lastSync").textContent = "SYNC: " + new Date().toLocaleTimeString();
}

// ── INIT ───────────────────────────────────────────────────────────────────
document.addEventListener("DOMContentLoaded", () => {
    loadDeviceName();
    refreshAll();
    checkSecurityStatus();
    setInterval(() => {
        refreshAll();
        const box = document.getElementById("logBox");
        if (box && box.style.display !== "none") loadLogs();
    }, 5000);
});
</script>
</body>
</html>
)raw";

    return html + js;
}
#endif