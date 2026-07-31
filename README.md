# cs2-rayoX

Herramienta educativa de **escritura de memoria externa** para **Counter-Strike 2** que activa el **X-Ray** (glow en todos los jugadores a través de las paredes) parcheando una instrucción de `client.dll`.

<img width="1223" height="732" alt="image" src="https://github.com/user-attachments/assets/6930c485-6545-4ac7-bc64-bf8be0045062" />


> [!WARNING]
> ⚠️ **Esto podría ser perjudicial.** Su uso viola los Términos de Servicio de Valve y puede resultar en **baneo permanente de la cuenta por VAC**. Este proyecto es **exclusivamente con fines educativos** (aprender APIs de Windows como `ReadProcessMemory`, `WriteProcessMemory`, `Toolhelp32`). Usalo bajo tu propia responsabilidad y solo en entornos de práctica/offline.

---

## Cómo funciona

En `client.dll` hay una función que controla el X-Ray del juego. En una dirección específica contiene la instrucción `xor al, al` (`32 C0`). Cambiarla a `mov al, 1` (`B0 01`) activa el glow en **todos** los jugadores al instante; restaurarla lo apaga.

El programa:
1. Espera a que `cs2.exe` esté abierto.
2. Carga `client.dll`.
3. **Encuentra la dirección del X-Ray automáticamente** buscando una firma de bytes en la imagen del módulo (con comodines `?` para los bytes que varían entre builds).
4. Con **F1** alterna entre `B0 01` (activado) y `32 C0` (desactivado).

## Detección automática (sobrevive a parches de Valve)

La firma se define en [`config.h`](config.h:37):

```
32 C0                          ; xor al, al  ← el objetivo
4C 8B A4 24 C8 00 00 00        ; mov r12, [rsp+0xC8]
48 8B B4 24 C0 00 00 00        ; mov rsi, [rsp+0xC0]
48 8B 9C 24 D0 00 00 00        ; mov rbx, [rsp+0xD0]
```

- Si la función mantiene esa forma en un build futuro, el programa la encuentra solo (**cero mantenimiento**).
- Si Valve cambia la función, el programa avisa `Firma no encontrada` y usa el **offset fijo** `DESPLAZAMIENTO_XRAY` (0xC12629, Build 14173) como respaldo.

### Redescubrir la firma con Cheat Engine (cuando Valve actualice)

1. En la consola de CS2: `spec_show_xray 1`.
2. En Cheat Engine: escanear el valor, cambiarlo (ej. a 1234), "next scan" → queda una dirección.
3. Click derecho → *Find out what accesses this address* (F5) → aparece `cmp dword ptr [rax], 00` en `client.dll+C12068`.
4. En el diagrama, el `xor al, al` al que confluyen las flechas es `client.dll+C12629`.
5. Actualizar `FIRMA_XRAY`/`MASCARA_FIRMA_XRAY` (o `DESPLAZAMIENTO_XRAY`) en [`config.h`](config.h).

## Requisitos

- Windows 10/11 **x64**.
- Visual Studio 2022+ (solo para compilar; el ejecutable no lo necesita).
- Ejecutar **como Administrador** (necesario para `PROCESS_ALL_ACCESS`).

## Compilación (sin Visual Studio)

```bat
build.bat
```

Genera `cs2-rayoX.exe` en la raíz.

## CI / Release en GitHub

El repositorio incluye un workflow de **GitHub Actions** (`.github/workflows/build.yml`) que:

1. Compila automáticamente en `windows-latest` con `build.bat` (cl.exe).
2. Genera `cs2-rayoX.exe` y `cs2-rayoX.zip`.
3. Los sube como artifact.
4. **Publica la Release** con el `.exe` y el `.zip` cuando pusheás un tag `vX.Y`.

### Cómo publicar la versión 1.0

```bash
git tag v1.0
git push origin v1.0
```

El workflow publica la Release automáticamente con `cs2-rayoX.exe` y `cs2-rayoX.zip`.

> También podés disparar el build manualmente desde la pestaña **Actions → Build cs2-rayoX → Run workflow** (solo compila y sube artifacts, sin publicar Release).

## Uso

1. Ejecutar como Administrador (con CS2 abierto).
2. **F1** → X-Ray activado/desactivado.

## Medidas de discreción

- **Restauración al salir**: al cerrar el programa (Ctrl+C o cierre de la ventana) se restauran los bytes originales (`32 C0`) en la dirección modificada.
- **Verificación previa**: antes de escribir se confirma que la dirección contenga el byte esperado (`0x32`), evitando modificar memoria equivocada si el offset quedó desactualizado.
- **Consola ocultable**: con `OCULTAR_CONSOLA = true` en [`config.h`](config.h:22) la ventana de consola se oculta al iniciar.

## Estructura

```
├── main.cpp               → lógica principal: espera el juego, busca la firma, toggle F1
├── config.h               → toda la configuración (proceso, tecla, offset, firma, discreción)
├── proceso_externo.h/.cpp → ProcesoExterno: RAII sobre OpenProcess + Read/WriteProcessMemory
├── gestor_modulos.h/.cpp  → GestorDeModulos: enumeración de módulos vía Toolhelp32
├── buscador_firmas.h/.cpp → BuscadorDeFirmas: búsqueda de firmas de bytes con máscara
├── build.bat              → compilación con cl.exe (sin Visual Studio)
└── .github/workflows/     → CI de GitHub Actions (compila y publica Releases)
```

---

Valve: arregla tu juego de mierda.
