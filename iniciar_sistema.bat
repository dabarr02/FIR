@echo off
echo [*] Iniciando Nginx...
start "" "%~dp0tools\nginx\nginx.exe" -p "%~dp0tools\nginx"

echo [*] Iniciando RadioAccess Backend...
cd build
start ./Release/RadioAccessTFG.exe

echo [*] Abriendo navegador en http://localhost...
timeout /t 3 /nobreak >nul
start "" "http://localhost"

echo [!] Todo listo. Abre http://localhost en tu navegador.
pause