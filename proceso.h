#pragma once

#include <Windows.h>

// ============================================================
//  Proceso: envoltura RAII sobre un proceso externo.
//  Abre el proceso, permite leer/escribir memoria y cierra el
//  handle automáticamente al destruirse.
//  lococoi
// ============================================================
class Proceso
{
public:
    Proceso();
    ~Proceso();

    // No copiable
    Proceso(const Proceso&) = delete;
    Proceso& operator=(const Proceso&) = delete;

    // Abre el proceso buscándolo por nombre de ejecutable (case-insensitive).
    bool Abrir(const char* nombre, DWORD acceso);

    // Cierra el handle si está abierto.
    void Cerrar();

    bool  EstaAbierto() const { return m_handle != nullptr; }
    DWORD ObtenerPid() const { return m_pid; }
    DWORD ObtenerUltimoError() const { return m_ultimoError; }

    // Lee 'tam' bytes desde 'direccion'. Devuelve true solo si leyó todo.
    bool LeerMemoria(uintptr_t direccion, void* buffer, SIZE_T tam, SIZE_T* leidos = nullptr) const;

    // Escribe 'tam' bytes en 'direccion'. Devuelve true solo si escribió todo.
    bool EscribirMemoria(uintptr_t direccion, const void* buffer, SIZE_T tam, SIZE_T* escritos = nullptr) const;

private:
    HANDLE m_handle;
    DWORD  m_pid;
    mutable DWORD m_ultimoError;
};
