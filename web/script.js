let lastText = "";
let contactCount = 0;

async function fetchData() {
    try {
        const res = await fetch('/api/status');
        const data = await res.json();
        const btn = document.getElementById('toggleBtn');
        const sText = document.getElementById('statusText');
        const sDot = document.getElementById('statusDot');

        if (data.isProcessing) {
            btn.innerText = "PAUSAR MOTOR";
            btn.className = "btn btn-sm btn-outline-danger me-3 fw-bold";
            sText.innerText = "Motor de IA Conectado";
            sText.className = "small text-uppercase text-success";
            sDot.style.backgroundColor = "#2ecc71"; // Verde
        } else {
            btn.innerText = "REANUDAR MOTOR";
            btn.className = "btn btn-sm btn-success me-3 fw-bold";
            sText.innerText = "Motor Pausado";
            sText.className = "small text-uppercase text-danger";
            sDot.style.backgroundColor = "#e74c3c"; 
        }

        const tBox = document.getElementById('transcription');
        if (data.transcription && data.transcription !== lastText) {
            tBox.innerText = data.transcription;
            tBox.scrollTop = tBox.scrollHeight;
            lastText = data.transcription;
        }

        if (data.contacts.length !== contactCount) {
            renderContacts(data.contacts);
            contactCount = data.contacts.length;
        }
    } catch (e) {
        console.error("Error de conexión con el motor C++");
    }
}

function renderContacts(contacts) {
    const list = document.getElementById('contactsList');
    list.innerHTML = ""; 
    
    
    [...contacts].reverse().forEach(c => {
        const qrzLink = `https://www.qrz.com/db/${c.call}`;
        
        
        const badgeColor = c.qrz_valid ? "bg-success" : "bg-info text-dark";
        const badgeText = c.qrz_valid ? "QRZ VERIFICADO" : "UIT RECONOCIDO";

        list.innerHTML += `
            <div class="contact-card p-3 shadow-sm mb-2 border ${c.qrz_valid ? 'border-success' : 'border-info'}" style="background-color: #1a1d20; border-radius: 6px;">
                <div class="d-flex justify-content-between align-items-center">
                    <a href="${qrzLink}" target="_blank" class="text-decoration-none">
                        <strong class="text-warning fs-5">${c.call}</strong>
                    </a>
                    <span class="badge ${badgeColor} extra-small" style="font-size: 0.65rem;">${badgeText}</span>
                </div>
                <div class="d-flex justify-content-between align-items-center mt-2">
                    <div class="small text-white">${c.name}</div>
                    <span class="badge bg-secondary extra-small" style="font-size: 0.7rem;">${c.mode || 'SSB'}</span>
                </div>
                <div class="text-muted extra-small mt-1" style="font-size: 0.75rem">${c.loc}</div>
                <div class="text-end extra-small text-muted mt-1" style="font-size: 0.65rem;">
                    ${c.date || ""} | ${c.time ? `${c.time.slice(0,2)}:${c.time.slice(2,4)}:${c.time.slice(4,6)}` : ""}
                </div>
            </div>
        `;
    });
}

async function sendTX() {
    const input = document.getElementById('txInput');
    const btn = document.querySelector('button[onclick="sendTX()"]');
    if (!input.value) return;

    btn.disabled = true;
    btn.innerText = "ENVIANDO...";

    try {
        await fetch('/api/transmit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ text: input.value })
        });
        
        input.value = ""; 
        btn.innerText = "ENVIAR";
        btn.classList.replace('btn-warning', 'btn-success');
        
        setTimeout(() => {
            btn.disabled = false;
            btn.innerText = "ENVIAR";
            btn.classList.replace('btn-success', 'btn-warning');
        }, 1000);
    } catch (e) {
        btn.innerText = "ERROR";
        btn.classList.replace('btn-warning', 'btn-danger');
    }
}

async function toggleEngine() {
    try {
        const res = await fetch('/api/toggle', { method: 'POST' });
        
        if (res.status === 403) {
            alert("⚠️ Configuración requerida: Por favor, introduce tu indicativo en los ajustes antes de arrancar el motor.");
            const settingsPanel = document.querySelector('.station-settings');
            if (settingsPanel) settingsPanel.open = true;
            return;
        }
        fetchData();
    } catch (e) {
        console.error("No se pudo conectar con el motor de radio.");
    }
}

async function addManualCallsign() {
    const input = document.getElementById('manualCallInput');
    const callsign = input.value.trim().toUpperCase();
    if (!callsign) return;

    try {
        // Delegación de la validación sintáctica y de prefijos al backend de C++
        const res = await fetch('/api/lookup', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ call: callsign })
        });

        if (res.ok) {
            input.value = ""; // Limpiamos la caja de texto tras el éxito
        } else {
            alert("El indicativo no cumple la sintaxis internacional UIT ni figura en la base de datos local del motor.");
        }
    } catch (e) {
        console.error("Error en la validación manual");
    }
}

async function shutdownSystem() {
    const confirmacion = confirm("⚠️ Vas a detener el motor de radio y cerrar Nginx. ¿Estás seguro?");
    if (confirmacion) {
        try {
            await fetch('/api/shutdown', { method: 'POST' });
            document.body.innerHTML = `
                <div class="container vh-100 d-flex align-items-center justify-content-center">
                    <div class="text-center p-5 bg-dark rounded border border-secondary shadow-lg">
                        <h1 class="display-1 mb-4">📡</h1>
                        <h2 class="text-white mb-3">Sistema Desconectado</h2>
                        <p class="text-muted mb-4">El motor RadioAccess y el servidor Nginx se han cerrado correctamente.</p>
                        <button class="btn btn-outline-warning" onclick="location.reload()">REINTENTAR CONEXIÓN</button>
                    </div>
                </div>`;
        } catch (e) {
            alert("El servidor ya no responde. Es probable que ya se haya cerrado.");
        }
    }
}

async function loadAudioDevices() {
    try {
        const res = await fetch('/api/devices');
        const devices = await res.json();
        const select = document.getElementById('cfgAudioOut');
        select.innerHTML = ""; 
        devices.forEach(d => {
            select.innerHTML += `<option value="${d.id}">${d.name}</option>`;
        });
    } catch (e) { console.error("Error cargando dispositivos de audio"); }
}

async function saveStationSettings() {
    const settings = {
        user: document.getElementById('cfgUser').value,
        pass: document.getElementById('cfgPass').value,
        myCall: document.getElementById('cfgMyCall').value,
        band: document.getElementById('cfgBand').value,
        mode: document.getElementById('cfgMode').value,
    };
    const audioVal = parseInt(document.getElementById('cfgAudioOut').value);
    if (!isNaN(audioVal) && audioVal >= 0) {
        settings.deviceId = audioVal;
    }

    try {
        const res = await fetch('/api/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(settings)
        });

        if (res.ok) {
            alert("Configuración aplicada.");
            const settingsPanel = document.querySelector('.station-settings');
            if (settingsPanel) settingsPanel.open = false;
            return;
        }
        alert("No se pudo aplicar la configuración.");
    } catch (e) {
        alert("Error al conectar con el motor.");
    }
}

function downloadReport() {
    window.location.href = '/api/report';
}

document.getElementById('txInput').addEventListener('keydown', function(event) {
    if (event.key === 'Enter') {
        event.preventDefault();
        sendTX();
    }
});

window.onload = async () => {
    try {
        const res = await fetch('/api/settings');
        const data = await res.json();
        await loadAudioDevices();
        
        document.getElementById('cfgUser').value = data.user || "";
        document.getElementById('cfgPass').value = data.pass || "";
        document.getElementById('cfgMyCall').value = data.myCall || "";
        document.getElementById('cfgBand').value = data.band || "2M";
        document.getElementById('cfgMode').value = data.mode || "SSB";
        document.getElementById('cfgAudioOut').value = data.deviceId || -1;

        if (data.needsConfig) {
            alert("⚠️ Configuración inicial requerida. Por favor, introduce tu indicativo.");
        }
    } catch (e) {
        console.error("Error cargando ajustes iniciales");
    }
};

setInterval(fetchData, 400);