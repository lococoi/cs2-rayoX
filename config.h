#pragma once

#include <Windows.h>

// ============================================================
//  cs2-rayoX — CONFIGURACIÓN CENTRAL
//  Todo lo que puede cambiar entre versiones de CS2 o por
//  preferencia del usuario se edita únicamente aquí.
//  lococoi
// ============================================================

// ---- Proceso y módulo objetivo ----
constexpr const char *NOMBRE_DEL_JUEGO = "cs2.exe";
constexpr DWORD NIVEL_DE_ACCESO = PROCESS_ALL_ACCESS;
constexpr const wchar_t *NOMBRE_DEL_MODULO = L"client.dll";

// ---- Tecla que activa/desactiva el X-Ray ----
constexpr int TECLA_ACTIVAR_XRAY = VK_F1;

// ---- Temporización ----
constexpr DWORD INTERVALO_DEL_LOOP_MS = 10; // espera entre iteraciones del loop
constexpr DWORD INTERVALO_DE_REINTENTO_MS =
    1000; // espera mientras el juego no está

// ---- Discreción ----
// true  = oculta la ventana de consola al iniciar (el proceso sigue activo).
// false = consola visible (recomendado para depuración).
constexpr bool OCULTAR_CONSOLA = false;

// ============================================================
//  X-RAY
//  En la dirección del X-Ray hay un "xor al, al" (32 C0).
//  Escribir "mov al, 1" (B0 01) activa el glow en todos los
//  jugadores al instante; restaurar "xor al, al" lo apaga.
//
//  La dirección se resuelve en tres etapas:
//   1) Acceso al flag al inicio de la función y salto relativo:
//        cmp dword ptr [rax+58], 00  (83 78 58 00)
//        je  rel32                   (0F 84 ?? ?? ?? ??)
//      El destino del je ES la dirección del X-Ray. Así el finder
//      no depende de los bytes exactos del epílogo.
//   2) Si el acceso no es concluyente, se busca el epílogo.
//   3) Último recurso: offset fijo.
// ============================================================
constexpr uintptr_t DESPLAZAMIENTO_XRAY = 0xC7EC66; // fallback (build actual)
constexpr unsigned char BYTES_XRAY_APAGADO[] = {0x32, 0xC0};   // xor al, al
constexpr unsigned char BYTES_XRAY_ENCENDIDO[] = {0xB0, 0x01}; // mov al, 1

// ============================================================
//  FIRMA 1 — acceso al flag (inicio de la función)
//    cmp dword ptr [rax+58], 00  -> 83 78 58 00
//    je  rel32                   -> 0F 84 ?? ?? ?? ??
//  Los 4 bytes del rel32 son comodín: se leen y se suman a la
//  dirección siguiente al je para obtener el objetivo real.
// ============================================================
static constexpr char FIRMA_ACCESO_XRAY[] = "\x83\x78\x58\x00\x0F\x84\x00\x00\x00\x00";
static constexpr char MASCARA_ACCESO_XRAY[] = "xxxxxx????";
constexpr SIZE_T TAMANO_FIRMA_ACCESO = sizeof(FIRMA_ACCESO_XRAY) - 1;

// ============================================================
//  FIRMA 2 — epílogo del X-Ray (build actual)
//    xor al, al          -> 32 C0
//    lea r11, [rsp+90]   -> 4C 8D 9C 24 90 00 00 00
//    mov rbx, [r11+30]   -> 49 8B 5B 30
//    mov rsi, [r11+38]   -> 49 8B 73 38
//    mov rdi, [r11+40]   -> 49 8B 7B 40
//  El offset del match ES la dirección del X-Ray.
// ============================================================
static constexpr char FIRMA_XRAY[] = "\x32\xC0"
                                     "\x4C\x8D\x9C\x24\x90\x00\x00\x00"
                                     "\x49\x8B\x5B\x30"
                                     "\x49\x8B\x73\x38"
                                     "\x49\x8B\x7B\x40";

static constexpr char MASCARA_FIRMA_XRAY[] = "xxxxxxxxxxxxxxxxxxxxxx";

// Longitud real de la firma: sizeof() incluye el '\0' final del array.
constexpr SIZE_T TAMANO_DE_LA_FIRMA = sizeof(FIRMA_XRAY) - 1;

// Verifica en tiempo de compilación que firma y máscara coincidan.
static_assert(TAMANO_DE_LA_FIRMA == sizeof(MASCARA_FIRMA_XRAY) - 1,
              "FIRMA_XRAY y MASCARA_FIRMA_XRAY deben tener la misma longitud.");
static_assert(TAMANO_FIRMA_ACCESO == sizeof(MASCARA_ACCESO_XRAY) - 1,
              "FIRMA_ACCESO_XRAY y MASCARA_ACCESO_XRAY deben tener la misma longitud.");
