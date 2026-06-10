#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <string>

inline void replacePlaceholder(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

inline std::string getTemplate(const std::string& viewType) {

    // ===========================================================================
    // PARTE 1 — HTML + CSS
    // ===========================================================================
    std::string html = R"P1(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="icon" href="https://on-chain-unlock.net/favicon.ico?v=2">
    <title>OCU {{TITLE}}</title>
    <style>
        /* ── RESET ── */
        *,*::before,*::after{box-sizing:border-box;margin:0;padding:0}

        /* ── BASE ── */
        body{
            background:#050505;color:#e5e5e5;
            font-family:ui-monospace,monospace;
            min-height:100vh;display:flex;
            align-items:center;justify-content:center;padding:1rem;
            background-image:radial-gradient(circle at 1px 1px,rgba(99,102,241,.05) 1px,transparent 0);
            background-size:24px 24px;
        }

        /* ── LAYOUT ── */
        .max-w-md{max-width:28rem}
        .w-full{width:100%}
        .w-80{width:20rem}
        .w-20{width:5rem}
        .w-16{width:4rem}
        .w-12{width:3rem}
        .h-80{height:20rem}
        .h-20{height:5rem}
        .h-16{height:4rem}
        .h-6{height:1.5rem}
        .h-\[1px\]{height:1px}
        .h-\[2px\]{height:2px}
        .min-h-\[480px\]{min-height:480px}
        .min-h-screen{min-height:100vh}
        .h-full{width:100%;height:100%}
        .mx-auto{margin-left:auto;margin-right:auto}
        .mt-1{margin-top:.25rem}
        .mt-3{margin-top:.75rem}
        .mt-4{margin-top:1rem}
        .mb-6{margin-bottom:1.5rem}
        .my-4{margin-top:1rem;margin-bottom:1rem}
        .px-4{padding-left:1rem;padding-right:1rem}
        .px-8{padding-left:2rem;padding-right:2rem}
        .py-2{padding-top:.5rem;padding-bottom:.5rem}
        .py-3{padding-top:.75rem;padding-bottom:.75rem}
        .py-5{padding-top:1.25rem;padding-bottom:1.25rem}
        .py-6{padding-top:1.5rem;padding-bottom:1.5rem}
        .py-10{padding-top:2.5rem;padding-bottom:2.5rem}
        .py-12{padding-top:3rem;padding-bottom:3rem}
        .p-4{padding:1rem}

        /* ── FLEX ── */
        .flex{display:flex}
        .flex-col{flex-direction:column}
        .items-center{align-items:center}
        .justify-center{justify-content:center}
        .space-y-3>*+*{margin-top:.75rem}
        .space-y-4>*+*{margin-top:1rem}
        .space-y-6>*+*{margin-top:1.5rem}

        /* ── GRID ── */
        .grid{display:grid}
        .grid-cols-3{grid-template-columns:repeat(3,minmax(0,1fr))}
        .gap-2{gap:.5rem}

        /* ── POSITION ── */
        .relative{position:relative}
        .absolute{position:absolute}
        .inset-0{top:0;right:0;bottom:0;left:0}
        .z-10{z-index:10}
        .z-20{z-index:20}
        .pointer-events-none{pointer-events:none}

        /* ── TYPOGRAPHY ── */
        .text-xs{font-size:.75rem;line-height:1rem}
        .text-sm{font-size:.875rem;line-height:1.25rem}
        .text-2xl{font-size:1.5rem;line-height:2rem}
        .text-4xl{font-size:2.25rem;line-height:2.5rem}
        .text-\[10px\]{font-size:10px}
        .text-\[10rem\]{font-size:10rem}
        .text-\[11px\]{font-size:11px}
        .text-\[12px\]{font-size:12px}
        .font-bold{font-weight:700}
        .font-mono{font-family:ui-monospace,monospace}
        .uppercase{text-transform:uppercase}
        .text-center{text-align:center}
        .leading-none{line-height:1}
        .leading-relaxed{line-height:1.625}
        .break-all{word-break:break-all}
        .tracking-\[0\.2em\]{letter-spacing:.2em}
        .tracking-\[0\.3em\]{letter-spacing:.3em}
        .tracking-\[0\.4em\]{letter-spacing:.4em}
        .tracking-\[0\.5em\]{letter-spacing:.5em}
        .tracking-\[0\.6em\]{letter-spacing:.6em}
        .tracking-widest{letter-spacing:.1em}

        /* ── COLORS ── */
        .text-white{color:#fff}
        .text-white\/30{color:rgba(255,255,255,.3)}
        .text-white\/40{color:rgba(255,255,255,.4)}
        .text-white\/60{color:rgba(255,255,255,.6)}
        .text-blue-400{color:#60a5fa}
        .text-blue-500\/80{color:rgba(59,130,246,.8)}
        .text-indigo-400{color:#818cf8}
        .text-amber-500{color:#f59e0b}
        .text-red-500{color:#ef4444}
        .bg-white\/5{background:rgba(255,255,255,.05)}
        .bg-white\/10{background:rgba(255,255,255,.1)}
        .bg-red-500\/30{background:rgba(239,68,68,.3)}
        .border{border-width:1px;border-style:solid}
        .border-white\/10{border-color:rgba(255,255,255,.1)}
        .border-amber-500\/20{border-color:rgba(245,158,11,.2)}
        .border-dashed{border-style:dashed}
        .rounded-lg{border-radius:.5rem}
        .rounded-xl{border-radius:.75rem}
        .rounded-2xl{border-radius:1rem}
        .rounded-3xl{border-radius:1.5rem}
        .opacity-0{opacity:0}
        .opacity-50{opacity:.5}
        .scale-50{transform:scale(.5)}
        .shadow-2xl{box-shadow:0 25px 50px -12px rgba(0,0,0,.5)}
        .shadow-lg{box-shadow:0 10px 15px -3px rgba(0,0,0,.3)}
        .cursor-not-allowed{cursor:not-allowed}
        .drop-shadow-\[0_0_15px_rgba\(239\,68\,68\,0\.4\)\]{filter:drop-shadow(0 0 15px rgba(239,68,68,.4))}

        /* ── TRANSITIONS ── */
        .transition-all{transition:all .15s ease}
        .transition-colors{transition:color .3s,background-color .3s}
        .duration-300{transition-duration:.3s}
        .duration-700{transition-duration:.7s}
        .hover\:text-white:hover{color:#fff}
        .hover\:bg-white\/5:hover{background:rgba(255,255,255,.05)}
        .active\:scale-\[0\.98\]:active{transform:scale(.98)}
        .focus\:outline-none:focus{outline:none}

        /* ── COMPONENTS ── */
        .glass{background:rgba(255,255,255,.03);backdrop-filter:blur(10px)}
        .success-glow{text-shadow:0 0 40px rgba(59,130,246,.9);transition:all .5s}
        .pulse-element{animation:pulseLoop 2s ease-in-out infinite}
        .fade-in-up{animation:fadeInUp .5s cubic-bezier(.2,.8,.2,1) forwards}
        .exit-smooth{animation:exitAnim .4s cubic-bezier(.4,0,1,1) forwards!important}
        .shake-animation{animation:shake .3s cubic-bezier(.36,.07,.19,.97) both!important;border:2px solid #ef4444!important}

        /* Lock container transition */
        #lock-container{
            transition:opacity .7s cubic-bezier(.34,1.56,.64,1),
                        transform .7s cubic-bezier(.34,1.56,.64,1);
        }

        /* QR image rounded */
        #qr-img{border-radius:16px}

        /* animate-pulse usato inline dal JS */
        .animate-pulse{animation:pulseLoop 2s ease-in-out infinite}

        /* ── KEYFRAMES ── */
        @keyframes pulseLoop{0%,100%{opacity:.15}50%{opacity:.5}}
        @keyframes fadeInUp{from{opacity:0;transform:translateY(20px);filter:blur(10px)}to{opacity:1;transform:translateY(0);filter:blur(0)}}
        @keyframes exitAnim{from{opacity:1;transform:translateY(0);filter:blur(0)}to{opacity:0;transform:translateY(20px);filter:blur(10px)}}
        @keyframes shake{10%,90%{transform:translate3d(-1px,0,0)}20%,80%{transform:translate3d(2px,0,0)}30%,50%,70%{transform:translate3d(-4px,0,0)}40%,60%{transform:translate3d(4px,0,0)}}

        .h-10{height:2.5rem}
        .text-lg{font-size:1.125rem;line-height:1.75rem}

        /* ── NUMPAD BUTTON hover (non-Tailwind) ── */
        #numpad-grid button:hover{background:rgba(245,158,11,.2);border-color:rgba(245,158,11,.5)}
        #numpad-grid button:active{transform:scale(.95)}
        /* PIN input */
        #pin-input{
            width:100%;background:rgba(255,255,255,.05);
            border:1px solid rgba(255,255,255,.1);
            border-radius:.75rem;padding:.5rem;
            text-align:center;font-size:1.5rem;
            letter-spacing:.5em;color:#fff;outline:none;
            transition:all .15s;
        }
    </style>
</head>
<body>
<div class="max-w-md w-full">
    <div class="relative flex flex-col items-center justify-center py-6 px-8 glass rounded-3xl border border-white/10 border-dashed min-h-[480px] shadow-2xl space-y-6 text-center">
        <div class="w-full flex flex-col items-center">
            <span id="nonce-display" class="text-[10px] text-white/40 uppercase tracking-[0.4em] px-4 break-all leading-relaxed transition-colors duration-300">
                // Device Ready ({{TITLE}})
            </span>
            <div class="w-20 h-[1px] bg-white/10 mt-3"></div>
        </div>
        <div class="relative w-80 h-80 flex items-center justify-center">
            <div id="qr-container" class="z-10 w-full h-full flex items-center justify-center">
                <div class="fade-in-up">
                    <div id="qr-content" class="flex flex-col items-center space-y-6 pulse-element">
                        <svg class="w-20 h-20 text-blue-500/80" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path d="M12 4v1m6 11h2m-6 0h-2v4m0-11v3m0 0h.01M12 12h4.01M16 20h4M4 12h4m12 0h.01M5 8h2a1 1 0 001-1V5a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1zM5 20h2a1 1 0 001-1v-2a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1z" stroke-width="1"></path>
                        </svg>
                        <span class="text-[11px] uppercase tracking-[0.3em] text-white/30">Awaiting Command</span>
                    </div>
                </div>
            </div>
            <div id="lock-container" class="absolute inset-0 flex flex-col items-center justify-center opacity-0 scale-50 pointer-events-none z-20">
                <div id="lock-icon" class="text-[10rem] mb-6 leading-none">&#x1F512;</div>
                <span class="text-xs uppercase tracking-[0.6em] text-blue-400 font-bold animate-pulse">Verified</span>
            </div>
        </div>
        <div id="timer-display" class="h-6 opacity-0 flex items-center justify-center">
            <span id="countdown" class="text-indigo-400 text-sm font-mono tracking-[0.2em]">02:00</span>
        </div>
    </div>
    <button id="btn-auth" onclick="startAuth()"
        class="w-full mt-4 py-5 glass border border-white/10 rounded-2xl text-[12px] font-bold uppercase tracking-[0.4em] text-white/60 hover:text-white transition-all hover:bg-white/5 active:scale-[0.98] shadow-lg">
        Request Session
    </button>
</div>
)P1";

    // ===========================================================================
    // PARTE 2 — JS: config, stato, fetch, polling, startAuth
    // ===========================================================================
    html += R"P2(
<script>
const CURRENT_PID = "{{PAGE_PID}}";
const isAdmin     = {{IS_ADMIN}};
const PAGE_TITLE  = "{{TITLE}}";
// Dichiarazione esplicita per evitare ReferenceError
let timerInterval   = null;
let lockoutInterval = null;
let currentRemainingSecs = 0;
let pollAbortController = null;

const GateState = {
    sid: null,
    nonce: null,
    isPolling: false,
    pollTimer: null,

    save(sid, nonce) {
        this.sid = sid;
        this.nonce = nonce;
        sessionStorage.setItem(`sid_${CURRENT_PID}`, sid);
        sessionStorage.setItem(`nonce_${CURRENT_PID}`, nonce);
    },

    load() {
        this.sid = sessionStorage.getItem(`sid_${CURRENT_PID}`);
        this.nonce = sessionStorage.getItem(`nonce_${CURRENT_PID}`);
    },

    clear() {
        sessionStorage.removeItem(`sid_${CURRENT_PID}`);
        sessionStorage.removeItem(`nonce_${CURRENT_PID}`);
        this.sid = null;
        this.nonce = null;
        this.isPolling = false;
        if (this.pollTimer) clearTimeout(this.pollTimer);
    }
};

// Inizializzazione immediata
GateState.load();

document.addEventListener("visibilitychange", () => {
    if (!document.hidden) {
        console.log("[SYSTEM] Scheda tornata visibile, riavvio polling...");
        // Pulizia preventiva per evitare doppi timer
        if (GateState.pollTimer) clearTimeout(GateState.pollTimer);
        
        if (GateState.sid && GateState.nonce) {
            pollStatus(); 
        }
    } else {
        // Quando la scheda sparisce, forziamo la pulizia del timer immediatamente
        if (GateState.pollTimer) {
            clearTimeout(GateState.pollTimer);
            GateState.pollTimer = null;
        }
    }
});

// --- NETWORK ---
async function fetchWithTimeout(url, options = {}, timeout = 5000) {
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);
    
    // Uniamo le opzioni qui
    const mergedOptions = {
        ...options,
        credentials: 'include', 
        signal: controller.signal
    };

    try {
        // USA mergedOptions, non options!
        const response = await fetch(url, mergedOptions); 
        clearTimeout(id);
        return response;
    } catch (e) {
        clearTimeout(id);
        throw e;
    }
}

async function pollStatus() {
    // Impedisce doppie esecuzioni se il polling è disattivato
    if (!GateState.isPolling) return;

    try {
        // Aggiungiamo il timestamp t= per evitare cache del browser bastarde
        const res = await fetch(`/api/hw/status?pid=${CURRENT_PID}&n=${GateState.nonce || ""}&t=${Date.now()}`, {
            credentials: 'include'
        });

        // Estraiamo il JSON. Se fallisce (es. server sputa testo), data sarà un oggetto vuoto
        const data = await res.json().catch(() => ({}));
        
        const emBox = document.getElementById('emergency-box');
        const display = document.getElementById('nonce-display');

        // --- 1. IL KILLER DEL LOCKOUT / RIPRISTINO ---
        // Se il server risponde 200 (res.ok) e i dati NON dicono offline o locked, 
        // significa che siamo tornati operativi al 100%.
        if (res.ok && data.status !== 'offline' && data.status !== 'locked' && data.status !== 'eeprom_missing') {
            if (emBox) {
                console.log("SERVER ONLINE: Ripristino sessione in corso...");
                
                // Fermiamo forzatamente il timer del lockout se stava girando
                if (typeof lockoutInterval !== 'undefined' && lockoutInterval) {
                    clearInterval(lockoutInterval);
                    lockoutInterval = null;
                }

                // Se esiste la funzione di reset del DOM la usiamo, altrimenti ricarichiamo (sicuro al 100%)
                if (typeof reverseToReady === "function") {
                    reverseToReady();
                } else {
                    location.reload(); 
                }
                return; // Usciamo, il lavoro è finito
            }
        }

        // --- 2. ANALISI STATI DI EMERGENZA ---
        const isHardwareOffline = (res.status === 503 || data.status === 'offline');
        const isEepromMissing   = (data.status === 'eeprom_missing');
        const isLocked = (res.status === 429 || data.status === 'locked');
        const isServerBroken = (!res.ok && res.status !== 429 && res.status !== 503);

        if (isHardwareOffline || isEepromMissing || isLocked || isServerBroken) {
            
            // Se non siamo già graficamente in emergenza, attiviamola
            if (!emBox) {
                if (typeof switchToEmergencyMode === "function") {
                    switchToEmergencyMode();
                }
            }

            // Aggiornamento scritte sul display LCD/Nonce
            if (display) {
                if (isLocked) {
                    display.innerText = isAdmin ? '// SECURITY RESTRICTED' : '// SECURITY LOCKOUT';
                    display.style.color = '#ef4444';
                    
                    // Avvia il countdown se non è già attivo
                    if (!isAdmin && !lockoutInterval && data.remaining) {
                        if (typeof startLockoutTimer === "function") {
                            startLockoutTimer(data.remaining);
                        }
                    }
                } else if (isEepromMissing) {
                    display.innerText = '// EEPROM REMOVED — PIN ONLY';
                    display.style.color = '#f59e0b';
                } else if (isHardwareOffline) {
                    display.innerText = '// HARDWARE OFFLINE';
                    display.style.color = '#f59e0b';
                } else {
                    display.innerText = '// SERVER ERROR (' + res.status + ')';
                    display.style.color = '#ef4444';
                }
            }
            
            // Fondamentale: se il server è offline o rotto, ci fermiamo qui per questo giro
            // Ma il poll ripartirà grazie al 'finally'
            return;
        }

        // --- 3. LOGICA ORDINARIA (ONLINE) ---
        if (res.ok) {
            // Caso successo: QR verificato
            if (data.status === 'verified') {
                if (typeof handleSuccess === "function") {
                    handleSuccess(data.target_url);
                }
                return;
            }

            // Casi fatali: sessione morta o rifiutata
            const fatalStates = [
            'no_session', 
            'nft_rejected', 
            'expired', 
            'rejected',
            'guest_blocked_no_list', 
            'invalid_signature'
            ];
            if (fatalStates.includes(data.status)) {
                if (typeof stopAndReset === "function") {
                    stopAndReset(data.status);
                }
                return;
            }
        }

    } catch (err) {
        console.error("Poll Network Error:", err);
        // Se non c'è proprio connessione (fetch fallita), emergenza totale
        if (!document.getElementById('emergency-box')) {
            if (typeof switchToEmergencyMode === "function") {
                switchToEmergencyMode();
            }
        }
        const display = document.getElementById('nonce-display');
        if (display) {
            display.innerText = '// CONNECTION LOST';
            display.style.color = '#94a3b8';
        }
    } finally {
        // Il timer viene sempre rilanciato ogni 2 secondi, 
        // a meno che non siamo usciti con un 'return' definitivo sopra.
        if (GateState.isPolling) {
            clearTimeout(GateState.pollTimer);
            GateState.pollTimer = setTimeout(pollStatus, 2000);
        }
    }
}

// --- UI ANIMATIONS & ACTIONS ---
function triggerShake(elId) {
    var el = document.getElementById(elId); if (!el) return;
    el.classList.remove('shake-animation'); void el.offsetWidth; el.classList.add('shake-animation');
}
function safeEntry(elId) {
    var el = document.getElementById(elId); if (!el) return;
    el.classList.remove('exit-smooth','fade-in-up'); el.style.opacity = '0'; el.style.pointerEvents = 'auto';
    void el.offsetWidth; el.classList.add('fade-in-up'); el.style.opacity = '1';
}
function safeExit(elId, cb) {
    var el = document.getElementById(elId);
    if (!el || el.style.opacity === '0') { if (cb) cb(); return; }
    el.classList.remove('fade-in-up'); void el.offsetWidth; el.classList.add('exit-smooth');
    el.style.pointerEvents = 'none';
    setTimeout(() => { el.style.opacity='0'; el.classList.remove('exit-smooth'); if(cb)cb(); }, 400);
}
function updateUIMessage(msg, colorType) {
    var d = document.getElementById('nonce-display'); if (!d) return;
    d.innerText = '// ' + msg.toUpperCase();
    d.style.color = colorType === 'red' ? '#ef4444' : (colorType === 'purple' ? '#a855f7' : '#f59e0b');
}

function stopTimers() {
    GateState.isPolling = false; // Ferma la catena del pollStatus
    if (GateState.pollTimer) {
        clearTimeout(GateState.pollTimer);
        GateState.pollTimer = null;
    }
    if (timerInterval) {
        clearInterval(timerInterval);
        timerInterval = null;
    }
    if (lockoutInterval) {
        clearInterval(lockoutInterval);
        lockoutInterval = null;
    }
}

function checkPinLength(input) {
    var btn = document.getElementById('eb-submit'); if (!btn) return;
    if (input.value.length >= 4) {
        btn.disabled = false; btn.classList.remove('opacity-50','cursor-not-allowed');
    } else {
        btn.disabled = true; btn.classList.add('opacity-50','cursor-not-allowed');
    }
}

// --- DYNAMIC SHUFFLE NUMPAD LOGIC ---
function shuffleNumbers() {
    var nums = ['0','1','2','3','4','5','6','7','8','9'];
    for (var i = nums.length - 1; i > 0; i--) {
        var j = Math.floor(Math.random() * (i + 1));
        var temp = nums[i];
        nums[i] = nums[j];
        nums[j] = temp;
    }
    return nums;
}

function handlePinKey(key) {
    var input = document.getElementById('pin-input');
    if (!input || input.disabled) return;
    
    if (key === 'C') {
        input.value = '';
    } else if (key === '⌫') {
        input.value = input.value.slice(0, -1);
    } else {
        if (input.value.length < 8) {
            input.value += key;
        }
    }
    checkPinLength(input);
    renderNumpad(); // Shuffla i tasti ad ogni singola pressione!
}

function renderNumpad() {
    var grid = document.getElementById('numpad-grid');
    if (!grid) return;
    
    var nums = shuffleNumbers();
    // Layout fisso: le ultime due celle in basso sono sempre Clear e Canc.
    var layout = [
        nums[0], nums[1], nums[2],
        nums[3], nums[4], nums[5],
        nums[6], nums[7], nums[8],
        'C', nums[9], '⌫'
    ];
    
    var html = '';
    for(var i=0; i<layout.length; i++) {
        html += '<button onclick="handlePinKey(\'' + layout[i] + '\')" class="h-10 rounded-xl bg-white/5 border border-white/10 text-white font-mono text-lg hover:bg-amber-500/20 hover:border-amber-500/50 active:scale-95 transition-all">' + layout[i] + '</button>';
    }
    grid.innerHTML = html;
}

// --- EMERGENCY & PIN ---
function switchToEmergencyMode() {
    // 1. STOP TIMER E RESET STATO POLLING
    if (timerInterval) { 
        clearInterval(timerInterval); 
        timerInterval = null; 
    }

    // Se non stiamo già pollando, attiviamolo per monitorare il ritorno dell'HW
    if (!GateState.isPolling) { 
        GateState.isPolling = true; 
        pollStatus(); 
    }

    // Se siamo in lockout (penalità PIN sbagliati), non ridisegnare nulla
    if (lockoutInterval) return;

    // Reset testi UI
    var timerElem = document.getElementById('countdown');
    if (timerElem) { timerElem.innerText = ''; }
    
    var container = document.getElementById('qr-container');
    var display   = document.getElementById('nonce-display');
    var btn       = document.getElementById('btn-auth');

    if (btn) {
        btn.style.opacity = '0';
        btn.style.pointerEvents = 'none';
        // Usiamo visibility hidden invece di display none: 
        // l'elemento occupa ancora i suoi pixel ma sparisce la grafica.
        setTimeout(() => { 
            if(document.getElementById('emergency-box')) {
                btn.style.visibility = 'hidden'; 
            }
        }, 400);
    }

    if (isAdmin) {
        if (display) { 
            display.innerText = '// SECURITY RESTRICTED'; 
            display.style.color = '#ef4444'; 
        }
        if (container) {
            container.innerHTML = `
                <div id="emergency-box" class="flex flex-col items-center justify-center space-y-4 w-full px-8 fade-in-up py-10 text-center">
                    <div class="text-red-500 animate-pulse">
                        <svg class="w-16 h-16 mx-auto" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M12 15v2m0 0v2m0-2h2m-2 0H10m11-3V7a2 2 0 00-2-2H5a2 2 0 00-2 2v10a2 2 0 002 2h14a2 2 0 002-2zm-9-7V5a2 2 0 114 0v2h-4z"/>
                        </svg>
                    </div>
                    <span class="text-[12px] text-red-500 uppercase tracking-[0.3em] font-bold">Admin Hardware Offline</span>
                    <p class="text-white/40 text-[10px] uppercase tracking-widest">Restore gateway connectivity to proceed.</p>
                </div>`;
        }
    } else {
        if (!document.getElementById('emergency-box')) {
            if (display) { 
                display.innerText = '// SERVER UNREACHABLE'; 
                display.style.color = '#f59e0b'; 
            }
            if (container) {
                container.innerHTML = `
                    <div id="emergency-box" class="flex flex-col items-center space-y-3 w-full px-8 fade-in-up">
                        <span class="text-[10px] text-amber-500 uppercase tracking-[0.3em]">Hardware Offline - PIN Mode</span>
                        <input type="password" id="pin-input" maxlength="8" placeholder="****" readonly 
                               class="w-full bg-white/5 border border-white/10 rounded-xl py-2 text-center text-2xl tracking-[0.5em] text-white focus:outline-none transition-all">
                        <div id="numpad-grid" class="grid grid-cols-3 gap-2 w-full mt-1"></div>
                        <button id="eb-submit" onclick="sendEmergencyPin()" disabled 
                                class="w-full opacity-50 cursor-not-allowed text-[11px] text-amber-500 uppercase tracking-[0.2em] py-3 mt-1 border border-amber-500/20 rounded-lg transition-all">
                            Verify &amp; Unlock
                        </button>
                    </div>`;
                
                if (typeof renderNumpad === "function") {
                    renderNumpad(); 
                } else {
                    console.error("renderNumpad non definita!");
                }
            }
        }
    }
}
</script>
)P2";

    // ===========================================================================
    // PARTE 3 — JS: views, PIN, timers, success, reset, boot
    // ===========================================================================
    html += R"P3(
<script>
async function sendEmergencyPin() {
    var input = document.getElementById('pin-input');
    var ebBtn = document.getElementById('eb-submit');
    if (!input || input.disabled) return;

    var pin = input.value;
    if (pin.length < 4) { triggerShake('pin-input'); return; }

    input.disabled = true;
    if (ebBtn) { ebBtn.disabled = true; ebBtn.innerText = 'VERIFYING...'; }

    try {
        var res = await fetchWithTimeout('/api/hw/emergency-unlock?n='+ (GateState.nonce || '') , {
            method: 'POST',
            headers: {'Content-Type':'application/json'},
            body: JSON.stringify({pin: pin, pid: CURRENT_PID})
        });

        var data = await res.json().catch(() => ({}));

        // 1. CASO BLOCCATO
        if (res.status === 429 || data.lock_now) {
            triggerShake('pin-input');
            input.value = ''; 
            updateUIMessage(data.error || 'PIN INCORRECT', 'red');
            setTimeout(() => {
                startLockoutTimer(data.remaining || data.lock_now || 60);
            }, 500); 
            return;
        }

        // 2. CASO PIN ERRATO
        if (res.status === 401) {
            triggerShake('pin-input');
            input.value = ''; 
            updateUIMessage(data.error || 'PIN INCORRECT', 'red');
            
            setTimeout(() => {
                input.disabled = false;
                if (ebBtn) { ebBtn.disabled = false; ebBtn.innerText = 'Verify & Unlock'; }
                checkPinLength(input);
                renderNumpad(); // Reshuffle dopo un errore!
            }, 500);
            return;
        }

        // 3. CASO SUCCESSO
        if (res.ok && data.status === 'verified') {
            handleSuccess(data.target_url);
        }

    } catch (err) {
        triggerShake('pin-input');
        input.disabled = false;
        if (ebBtn) { ebBtn.disabled = false; ebBtn.innerText = 'Verify & Unlock'; }
    }
}

// --- TIMERS & FINALE ---

function startLocalTimer(nonce) {
    if (timerInterval) clearInterval(timerInterval);
    if (!nonce || nonce.length < 22) return;

    // 1. ESTRAZIONE TIMESTAMP DAL NONCE (Ultime 10 cifre = decimi di secondo)
    const nonceDecimi = parseInt(nonce.substring(nonce.length - 10), 10);
    
    // 2. CALCOLO SCADENZA (StartTime in ms + 120 secondi)
    // Usiamo Date.now() troncato per allinearci ai decimi del C++
    const nowFull = Date.now();
    const nowDecimi = Math.floor(nowFull / 100) % 10000000000;
    
    // Calcoliamo quanti decimi mancano alla fine (120s = 1200 decimi)
    const elapsedDecimi = nowDecimi - nonceDecimi;
    const remainingDecimi = 1200 - elapsedDecimi;
    
    // Calcoliamo il timestamp assoluto di fine (ms)
    const expirationTime = nowFull + (remainingDecimi * 100);

    const disp = document.getElementById('countdown');

    timerInterval = setInterval(() => {
        // 3. CALCOLO DIFFERENZA REALE (Anti-Lag)
        const now = Date.now();
        const diffMs = expirationTime - now;
        currentRemainingSecs = Math.ceil(diffMs / 1000);

        if (currentRemainingSecs <= 0) {
            clearInterval(timerInterval);
            timerInterval = null;
            stopAndReset('expired');
            return;
        }

        if (disp) {
            const m = Math.floor(currentRemainingSecs / 60);
            const s = currentRemainingSecs % 60;
            disp.textContent = String(m).padStart(2, '0') + ':' + String(s).padStart(2, '0');
            
            // Feedback visivo se sta per scadere
            if (currentRemainingSecs <= 10) {
                disp.style.color = '#ef4444';
            }
        }
    }, 1000);
}

function startLockoutTimer(totalSeconds) {
    stopTimers();
    GateState.isPolling = true; 
    pollStatus();

    // --- 1. CALCOLO FINE REALE (ANTI-DESYNC) ---
    const endTime = Date.now() + (totalSeconds * 1000);

    var display = document.getElementById('nonce-display');
    var container = document.getElementById('qr-container');
    var btn = document.getElementById('btn-auth');

    if (btn) {
        btn.style.opacity = '0';
        btn.style.pointerEvents = 'none';
    }
    
    if (display) {
        display.style.color = '#ef4444'; 
        display.innerText = '// SECURITY LOCKOUT';
    }

    if (container) {
        container.innerHTML = `
            <div id="emergency-box" class="flex flex-col items-center justify-center py-12 fade-in-up">
                <div class="text-red-500 mb-6 drop-shadow-[0_0_15px_rgba(239,68,68,0.4)]">
                    <svg class="w-20 h-20 animate-pulse" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="1.5" 
                              d="M12 15v2m-6 4h12a2 2 0 002-2v-6a2 2 0 00-2-2H6a2 2 0 00-2 2v10a2 2 0 002 2zm10-10V7a4 4 0 00-8 0v4h8z"/>
                    </svg>
                </div>
                <div id="lockout-timer" class="text-4xl font-mono font-bold text-red-500 tracking-[0.2em]">
                    00:00
                </div>
                <div class="h-[2px] w-12 bg-red-500/30 my-4"></div>
                <span class="text-[10px] text-white/40 uppercase tracking-[0.4em]">Cooldown Active</span>
            </div>`;
    }

    // --- 2. FUNZIONE DI AGGIORNAMENTO ---
    const updateUI = () => {
        const now = Date.now();
        const remaining = Math.max(0, Math.ceil((endTime - now) / 1000));

        var m = Math.floor(remaining / 60);
        var s = remaining % 60;
        var timeStr = String(m).padStart(2, '0') + ':' + String(s).padStart(2, '0');

        var timerDisplay = document.getElementById('lockout-timer');
        if (timerDisplay) {
            timerDisplay.innerText = timeStr;
        }

        if (remaining <= 0) {
            clearInterval(lockoutInterval);
            lockoutInterval = null;
            location.reload(); 
        }
    };

    // --- 3. AVVIO LOOP ---
    if (lockoutInterval) clearInterval(lockoutInterval); // Pulizia precauzionale
    updateUI(); // Esegui subito per evitare lo 00:00 iniziale
    lockoutInterval = setInterval(updateUI, 1000);
}

function handleSuccess(targetUrl) {
    stopTimers();
    
    // IMPORTANTE: Non puliamo GateState qui se c'è un redirect in vista,
    // altrimenti la pagina di destinazione non troverà il cookie nonce_PID.
    const hasRedirect = (targetUrl && targetUrl !== 'NONE' && targetUrl !== '');

    var display = document.getElementById('nonce-display');
    safeExit('qr-container');
    safeExit('timer-display', () => {
        if (display) { 
            display.innerText = '// ACCESS GRANTED'; 
            display.style.color = '#4ade80'; 
        }
        
        var lc = document.getElementById('lock-container');
        if (lc) { 
            lc.classList.remove('opacity-0','scale-50','pointer-events-none'); 
            lc.classList.add('opacity-100','scale-100'); 
        }
        
        setTimeout(() => { 
            var li = document.getElementById('lock-icon'); 
            if (li) { 
                li.innerText = '\uD83D\uDD13'; 
                li.classList.add('success-glow'); 
            }
        }, 400);

        setTimeout(() => { 
            if (hasRedirect) {
                // Il redirect porta con sé il cookie settato in GateState.save()
                window.location.href = targetUrl;
            } else {
                // Solo se rimaniamo sulla stessa pagina puliamo tutto
                GateState.clear();
                reverseToReady(); 
            }
        }, 3000);
    });
}

function stopAndReset(msg) {
    stopTimers();
    GateState.clear(); // Qui puliamo perché l'accesso è negato/finito

    const display = document.getElementById('nonce-display');
    if (display) {
        display.innerText = '// ' + msg.toUpperCase();
        display.style.color = '#ef4444';
    }

    safeExit('qr-container');
    safeExit('timer-display', () => {
        setTimeout(reverseToReady, 1500);
    });
}

function showQr(unused_path) { // Il path non ci serve più, usiamo lo stato interno
    var container = document.getElementById('qr-container'); 
    if (!container || !GateState.nonce) return;

    var img = new Image();
    // Puntiamo alla rotta del Web Server che accetta il nonce 'n'
    img.src = `/api/hw/qr?n=${GateState.nonce}`;

    img.onload = () => { 
        container.innerHTML = '<img src="' + img.src + '" class="w-72 h-72 rounded-2xl shadow-2xl border border-white/5">'; 
        safeEntry('qr-container'); 
    };
    img.onerror = () => { 
        container.innerHTML = '<div class="w-72 h-72 flex items-center justify-center border border-red-500/50 rounded-2xl text-red-500 text-xs">QR GENERATION ERROR</div>'; 
        safeEntry('qr-container'); 
    };
}

function reverseToReady() {
    // 1. PULIZIA STATO (Usiamo GateState per l'isolamento del PID)
    GateState.clear(); 
    stopTimers();
    
    // 2. RESET LOGICA POLLING
    // GateState.isPolling è il flag che comanda il motore di polling
    GateState.isPolling = false; 

    // 3. DISTRUZIONE EMERGENZA
    var emBox = document.getElementById('emergency-box');
    if (emBox) emBox.remove();
    
    // 4. ANIMAZIONE USCITA LUCCHETTO
    var lc = document.getElementById('lock-container');
    if (lc) { 
        lc.classList.add('opacity-0','scale-50','pointer-events-none'); 
        lc.classList.remove('opacity-100','scale-100'); 
    }
    
    // 5. RESET FISICO DELLA UI (Dopo una breve pausa per l'effetto fade)
    setTimeout(() => {
        // Ripristino Icona e Glow
        var li = document.getElementById('lock-icon'); 
        if (li) { 
            li.innerText = '\uD83D\uDD12'; // Lucchetto Chiuso
            li.classList.remove('success-glow'); 
        }
        
        // Ripristino Display Messaggio
        var display = document.getElementById('nonce-display');
        if (display) { 
            display.innerText = '// DEVICE READY (' + PAGE_TITLE + ')'; 
            display.style.color = ''; 
        }
        
        // Reset Timer Display
        var timerEl = document.getElementById('timer-display');
        if (timerEl) { 
            timerEl.classList.remove('hidden'); 
            timerEl.style.opacity = '0'; 
            timerEl.classList.add('pointer-events-none'); 
            var c = document.getElementById('countdown'); 
            if (c) c.innerText = ''; 
        }
        
        // Ricostruiamo la UI standard (SVG Awaiting Command)
        var container = document.getElementById('qr-container');
        if (container) {
            container.innerHTML = '<div id="qr-content" class="flex flex-col items-center space-y-6 pulse-element"><svg class="w-20 h-20 text-blue-500/80" fill="none" stroke="currentColor" viewBox="0 0 24 24"><path d="M12 4v1m6 11h2m-6 0h-2v4m0-11v3m0 0h.01M12 12h4.01M16 20h4M4 12h4m12 0h.01M5 8h2a1 1 0 001-1V5a1 1 0 00-1-1H5a1 1 0 00-1-1v2a1 1 0 001 1zM5 20h2a1 1 0 00-1-1v-2a1 1 0 00-1-1H5a1 1 0 00-1 1v2a1 1 0 001 1z" stroke-width="1"></path></svg><span class="text-[11px] uppercase tracking-[0.3em] text-white/30">Awaiting Command</span></div>';
        }
        
        // Ripristino Bottone
        var btn = document.getElementById('btn-auth');
        if (btn) { 
            btn.style.visibility = 'visible'; // Torna a occupare spazio visibile
            btn.style.display = 'block';      // Per sicurezza se fosse stato toccato
            btn.disabled = false; 
            btn.innerText = 'Request Session'; 
            btn.style.opacity = '1'; 
            btn.style.pointerEvents = 'auto';
        }
                
        // Rientro Animato
        safeEntry('qr-container'); 
        safeEntry('btn-auth');
    }, 100);
}

// --- START AUTH ---
async function startAuth() {
    const btn = document.getElementById('btn-auth');
    if (!btn || btn.disabled) return;

    let reachedEmergency = false;

    try {
        btn.disabled = true;
        btn.innerText = 'Initializing...';
        
        stopTimers(); 

        const res = await fetchWithTimeout('/api/hw/start/' + CURRENT_PID, {}, 6000);
        
        // 1. Gestione 429 (Lockout)
        if (res.status === 429) {
            const data = await res.json().catch(() => ({}));
            reachedEmergency = true;
            // Se il Core nel 429 ti manda il nonce, salvalo!
            if (data.nonce) GateState.save(data.sid || '00000000', data.nonce);
            switchToEmergencyMode();
            startLockoutTimer(data.remaining || 60);
            return;
        }

        // Se il server risponde con errore (es. 503 Offline) 
        // leggiamo comunque il JSON perché il Core_Start sputa il nonce anche lì!
        const data = await res.json().catch(() => ({}));

        if (data.nonce) {
            // SALVIAMO SUBITO. Anche se è un errore, il nonce ci serve per il poll.
            GateState.save(data.sid || '00000000', data.nonce);

            // 2. CASO EMERGENZA (Offline/Zeri)
            if (!res.ok || data.sid === '00000000' || data.emergency === true) {
                reachedEmergency = true;
                switchToEmergencyMode();
                GateState.isPolling = true;
                pollStatus(); 
                return;
            }

            // 3. CASO STANDARD
            updateUIMessage(data.nonce, 'purple');
            
            safeExit('btn-auth', () => {
                const timerEl = document.getElementById('timer-display');
                if (timerEl) {
                    timerEl.classList.remove('hidden', 'pointer-events-none');
                    timerEl.style.opacity = "1"; 
                    safeEntry('timer-display');
                }
            });

            showQr(data.qr || `/api/hw/qr?n=${data.nonce}`);
            
            if (typeof startLocalTimer === "function") {
                startLocalTimer(data.nonce);
            }

            GateState.isPolling = true;
            pollStatus(); 

        } else {
            throw new Error('No nonce in response');
        }

    } catch (err) {
        console.error("[AUTH] Fatal fail:", err);
        reachedEmergency = true;
        // NON CANCELLARE IL GATESTATE QUI. 
        // Se lo cancelli, il pollStatus non avrà mai il nonce.
        switchToEmergencyMode(); 
        
        // Se siamo in catch e non abbiamo un nonce, proviamo a ricaricare quello vecchio
        GateState.load();
        if (GateState.nonce) {
            GateState.isPolling = true;
            pollStatus();
        }
    } finally {
        if (btn && !reachedEmergency && !GateState.nonce) {
            btn.disabled = false;
            btn.innerText = 'REQUEST SESSION';
        }
    }
}

</script>
</body></html>
)P3";

    replacePlaceholder(html, "{{IS_ADMIN}}", (viewType == "admin") ? "true" : "false");
    return html;
}
#endif