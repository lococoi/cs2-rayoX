#pragma once

#include <Windows.h>

// ============================================================
//  InfoModulo: datos de un módulo (DLL) cargado en el proceso.
//  lococoi
// ============================================================
struct InfoModulo
{
    uintptr_t direccionBase = 0;
    SIZE_T    tamano        = 0;
};

// ============================================================
//  GestorDeModulos: enumera los módulos de un proceso externo
//  (Toolhelp32) y permite encontrar uno por nombre.
// ============================================================
class GestorDeModulos
{
public:
    // Busca un módulo por nombre (no distingue mayúsculas).
    // Devuelve true y llena 'resultado' si lo encuentra.
    static bool ObtenerModulo(DWORD pid, const wchar_t* nombreDelModulo, InfoModulo& resultado);
};
