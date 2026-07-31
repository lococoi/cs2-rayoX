#pragma once

#include <Windows.h>

// ============================================================
//  ProcesoExterno: envuelve el acceso a un proceso externo
//  (el juego) para leer y escribir su memoria.
//  Abre el proceso, libera el handle automáticamente al salir
//  (RAII) y reporta los errores con claridad.
//  lococoi
// ============================================================
class ProcesoExterno
{
public:
    ProcesoExterno();
    ~ProcesoExterno();

    // No copiable (para no duplicar el handle del proceso).
    ProcesoExterno(const ProcesoExterno&) = delete;
    ProcesoExterno& operator=(const ProcesoExterno&) = delete;

    // Abre el proceso por nombre de ejecutable (no distingue mayúsculas).
    bool AbrirProceso(const char* nombreDelEjecutable, DWORD nivelDeAcceso);

    // Cierra el handle del proceso si estaba abierto.
    void CerrarProceso();

    bool  EstaAbierto() const { return m_handle != nullptr; }
    DWORD ObtenerPid() const { return m_pid; }
    DWORD ObtenerCodigoDeError() const { return m_ultimoError; }

    // Lee 'cantidadDeBytes' desde 'direccion'. Devuelve true solo si leyó todo.
    bool LeerMemoria(uintptr_t direccion, void* buffer, SIZE_T cantidadDeBytes, SIZE_T* bytesLeidos = nullptr) const;

    // Escribe 'cantidadDeBytes' en 'direccion'. Devuelve true solo si escribió todo.
    bool EscribirMemoria(uintptr_t direccion, const void* buffer, SIZE_T cantidadDeBytes, SIZE_T* bytesEscritos = nullptr) const;

private:
    HANDLE m_handle;
    DWORD  m_pid;
    mutable DWORD m_ultimoError;
};
