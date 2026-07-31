#pragma once

#include <Windows.h>

// ============================================================
//  cs2-rayoX — CONFIGURACIÓN CENTRAL
//  Todo lo que puede cambiar entre versiones de CS2 o por
//  preferencia del usuario se edita únicamente aquí.
//  lococoi
// ============================================================

// ---- Proceso y módulo objetivo ----
constexpr const char* NOMBRE_DEL_JUEGO      = "cs2.exe";
constexpr DWORD       NIVEL_DE_ACCESO       = PROCESS_ALL_ACCESS;
constexpr const wchar_t* NOMBRE_DEL_MODULO  = L"client.dll";

// ---- Tecla que activa/desactiva el X-Ray ----
constexpr int TECLA_ACTIVAR_XRAY = VK_F1;

// ---- Temporización ----
constexpr DWORD INTERVALO_DEL_LOOP_MS     = 10;    // espera entre iteraciones del loop
constexpr DWORD INTERVALO_DE_REINTENTO_MS = 1000;  // espera mientras el juego no está

// ---- Discreción ----
// true  = oculta la ventana de consola al iniciar (el proceso sigue activo).
// false = consola visible (recomendado para depuración).
constexpr bool OCULTAR_CONSOLA = false;

// ============================================================
//  X-RAY (técnica verificada con Cheat Engine — Build 14173)
//  En la dirección del X-Ray hay un "xor al, al" (32 C0).
//  Escribir "mov al, 1" (B0 01) activa el glow en todos los
//  jugadores al instante; restaurar "xor al, al" lo apaga.
// ============================================================
constexpr uintptr_t DESPLAZAMIENTO_XRAY = 0xC12629; // fallback (Build 14173)
constexpr unsigned char BYTES_XRAY_APAGADO[]   = { 0x32, 0xC0 }; // xor al, al
constexpr unsigned char BYTES_XRAY_ENCENDIDO[] = { 0xB0, 0x01 }; // mov al, 1

// ============================================================
//  FIRMA del X-Ray — detección automática en futuros builds
//  El "xor al, al" (32 C0) está seguido de:
//    mov r12, [rsp+0xC8]; mov rsi, [rsp+0xC0]; mov rbx, [rsp+0xD0]
//  El primer byte es comodín (?) porque puede variar entre builds.
//  El offset del match ES la dirección del X-Ray.
// ============================================================
static constexpr char FIRMA_XRAY[] =
    "\x00\xC0"
    "\x4C\x8B\xA4\x24\xC8\x00\x00\x00"
    "\x48\x8B\xB4\x24\xC0\x00\x00\x00"
    "\x48\x8B\x9C\x24\xD0\x00\x00\x00";

static constexpr char MASCARA_FIRMA_XRAY[] = "?xxxxxxxxxxxxxxxxxxxxxxxxx";

// Longitud real de la firma: sizeof() incluye el '\0' final del array.
constexpr SIZE_T TAMANO_DE_LA_FIRMA = sizeof(FIRMA_XRAY) - 1;

// Verifica en tiempo de compilación que firma y máscara coincidan.
static_assert(TAMANO_DE_LA_FIRMA == sizeof(MASCARA_FIRMA_XRAY) - 1,
              "FIRMA_XRAY y MASCARA_FIRMA_XRAY deben tener la misma longitud.");
