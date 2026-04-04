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
        await fetch('/api/toggle', { method: 'POST' });
        fetchData(); // Actualizamos la UI inmediatamente
    } catch (e) {
        console.error("No se pudo cambiar el estado del motor");
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

document.getElementById('txInput').addEventListener('keydown', function(event) {
    if (event.key === 'Enter') {
        event.preventDefault(); // Evita que el navegador haga cosas raras (como recargar)
        sendTX();
    }
});

// Ejecutar cada 400ms (balance perfecto entre fluidez y carga)
setInterval(fetchData, 400);