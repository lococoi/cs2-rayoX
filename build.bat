@echo off
rem ============================================================
rem  cs2-rayoX - build sin Visual Studio
rem  lococoi
rem ============================================================
setlocal

set "VCVARS="

rem Buscar vcvars64.bat en las instalaciones mas comunes.
for %%p in (
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\18\Insiders\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\18\Professional\VC\Auxiliary\Build\vcvars64.bat"
    "%ProgramFiles%\Microsoft Visual Studio\18\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
) do (
    if exist "%%~p" set "VCVARS=%%~p"
)

if "%VCVARS%"=="" (
    rem Fallback con vswhere.
    for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath 2^>nul`) do (
        if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%i\VC\Auxiliary\Build\vcvars64.bat"
    )
)

if "%VCVARS%"=="" (
    echo [ERROR] No se encontro Visual Studio ^(vcvars64.bat^).
    exit /b 1
)

call "%VCVARS%" >nul 2>&1

cl /nologo /O2 /W4 /WX /EHsc /utf-8 main.cpp proceso_externo.cpp gestor_modulos.cpp buscador_firmas.cpp /Fe:cs2-rayoX.exe /link /SUBSYSTEM:CONSOLE user32.lib
if errorlevel 1 (
    echo [ERROR] Compilacion fallida.
    exit /b 1
)

echo [OK] Generado: cs2-rayoX.exe
endlocal
