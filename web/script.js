let lastText = "";
let contactCount = 0;

async function fetchData() {
    try {
        const res = await fetch('/api/status');
        const data = await res.json();

        // --- Lógica del Botón de Pausa ---
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
            sDot.style.backgroundColor = "#e74c3c"; // Rojo
        }

        // 2. Actualizar Transcripción (lo que ya tenías)
        const tBox = document.getElementById('transcription');
        if (data.transcription && data.transcription !== lastText) {
            tBox.innerText = data.transcription;
            tBox.scrollTop = tBox.scrollHeight;
            lastText = data.transcription;
        }

        // 3. Actualizar Contactos (lo que ya tenías)
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
        // Creamos el enlace dinámico a QRZ
        const qrzLink = `https://www.qrz.com/db/${c.call}`;

        list.innerHTML += `
            <div class="contact-card p-3 shadow-sm">
                <div class="d-flex justify-content-between">
                    <a href="${qrzLink}" target="_blank" class="callsign-link">
                        <strong class="text-warning fs-5">${c.call}</strong>
                    </a>
                    <small class="text-muted">${new Date().toLocaleTimeString()}</small>
                </div>
                <div class="small">${c.name}</div>
                <div class="text-muted extra-small" style="font-size: 0.75rem">${c.loc}</div>
            </div>
        `;
    });
}

async function sendTX() {
    const input = document.getElementById('txInput');
    const btn = document.querySelector('button[onclick="sendTX()"]');
    
    if (!input.value) return;
    
    // Feedback visual: desactivamos el botón un segundo
    btn.disabled = true;
    btn.innerText = "ENVIANDO...";

    try {
        await fetch('/api/transmit', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ text: input.value })
        });
        
        input.value = ""; // Limpiamos
        btn.innerText = "ENVIADO";
        btn.classList.replace('btn-warning', 'btn-success');
        
        // Volvemos al estado normal tras 1 segundo
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
            // El servidor nos ha dicho que faltan credenciales
            alert("⚠️ Configuración requerida: Por favor, introduce tus datos de QRZ antes de arrancar el motor.");
            
            // Abrimos automáticamente el panel de configuración para ayudar al usuario
            const settingsPanel = document.querySelector('.station-settings');
            if (settingsPanel) settingsPanel.open = true;
            return;
        }

        fetchData(); // Si todo va bien, actualizamos la interfaz
    } catch (e) {
        console.error("No se pudo conectar con el motor de radio.");
    }
}

async function shutdownSystem() {
    // Pedimos confirmación para evitar desastres
    const confirmacion = confirm("⚠️ Vas a detener el motor de radio y cerrar Nginx. ¿Estás seguro?");

    if (confirmacion) {
        try {
            // Avisamos al backend
            await fetch('/api/shutdown', { method: 'POST' });

            // Mostramos una pantalla de despedida limpia
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
        select.innerHTML = ""; // Limpiar
        
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
        deviceId: parseInt(document.getElementById('cfgAudioOut').value)
    };

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

        if (res.status === 401) {
            alert("Datos incorrectos. Revisa usuario y contraseña de QRZ.");
        } else {
            alert("No se pudo aplicar la configuración.");
        }
    } catch (e) {
        alert("Error al conectar con el motor.");
    }
}

function downloadReport() {
    // Al llamar a esta URL, el navegador recibirá el "attachment" y empezará la descarga
    window.location.href = '/api/report';
}


document.getElementById('txInput').addEventListener('keydown', function(event) {
    if (event.key === 'Enter') {
        event.preventDefault(); // Evita que el navegador haga cosas raras (como recargar)
        sendTX();
    }
});

// Al cargar la página, recuperamos la configuración guardada
window.onload = async () => {
    try {
        const res = await fetch('/api/settings');
        const data = await res.json();
        await loadAudioDevices();
        
        document.getElementById('cfgUser').value = data.user || "";
        document.getElementById('cfgPass').value = data.pass || "";
        document.getElementById('cfgMyCall').value = data.myCall || "";
        document.getElementById('cfgBand').value = data.band || "2M";
        document.getElementById('cfgAudioOut').value = data.deviceId || -1;

        if (data.needsConfig) {
            alert("⚠️ Configuración inicial requerida. Por favor, introduce tus datos de QRZ.");
        }
    } catch (e) {
        console.error("Error cargando ajustes iniciales");
    }
};

// Ejecutar cada 400ms (balance perfecto entre fluidez y carga)
setInterval(fetchData, 400);