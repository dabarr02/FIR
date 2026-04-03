@echo off
echo [*] Iniciando Nginx...
start "" "%~dp0tools\nginx\nginx.exe" -p "%~dp0tools\nginx"

echo [*] Iniciando RadioAccess Backend...
cd build
start ./Release/RadioAccessTFG.exe

echo [!] Todo listo. Abre http://localhost en tu navegador.
pause