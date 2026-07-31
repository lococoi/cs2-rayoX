#include "proceso_externo.h"

#include <TlHelp32.h>
#include <cstring>

ProcesoExterno::ProcesoExterno()
    : m_handle(nullptr), m_pid(0), m_ultimoError(ERROR_SUCCESS)
{
}

ProcesoExterno::~ProcesoExterno()
{
    CerrarProceso();
}

bool ProcesoExterno::AbrirProceso(const char* nombreDelEjecutable, DWORD nivelDeAcceso)
{
    CerrarProceso();

    PROCESSENTRY32 entradaDeProceso{};
    entradaDeProceso.dwSize = sizeof(PROCESSENTRY32);

    HANDLE capturaDeProcesos = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
    if (capturaDeProcesos == INVALID_HANDLE_VALUE)
    {
        m_ultimoError = GetLastError();
        return false;
    }

    bool encontrado = false;
    if (Process32First(capturaDeProcesos, &entradaDeProceso))
    {
        // do-while: el primer proceso del snapshot también se compara.
        do
        {
            if (_stricmp(entradaDeProceso.szExeFile, nombreDelEjecutable) == 0)
            {
                encontrado = true;
                break;
            }
        } while (Process32Next(capturaDeProcesos, &entradaDeProceso));
    }

    CloseHandle(capturaDeProcesos);

    if (!encontrado)
    {
        m_ultimoError = ERROR_FILE_NOT_FOUND;
        return false;
    }

    HANDLE handleDelProceso = OpenProcess(nivelDeAcceso, FALSE, entradaDeProceso.th32ProcessID);
    if (handleDelProceso == nullptr)
    {
        m_ultimoError = GetLastError();
        return false;
    }

    m_handle = handleDelProceso;
    m_pid    = entradaDeProceso.th32ProcessID;
    m_ultimoError = ERROR_SUCCESS;
    return true;
}

void ProcesoExterno::CerrarProceso()
{
    if (m_handle != nullptr)
    {
        CloseHandle(m_handle);
        m_handle = nullptr;
    }
    m_pid = 0;
}

bool ProcesoExterno::LeerMemoria(uintptr_t direccion, void* buffer, SIZE_T cantidadDeBytes, SIZE_T* bytesLeidos) const
{
    if (!EstaAbierto() || direccion == 0 || buffer == nullptr || cantidadDeBytes == 0)
        return false;

    SIZE_T bytesLeidosReal = 0;
    if (!ReadProcessMemory(m_handle, reinterpret_cast<LPCVOID>(direccion), buffer, cantidadDeBytes, &bytesLeidosReal))
    {
        m_ultimoError = GetLastError();
        return false;
    }

    if (bytesLeidos)
        *bytesLeidos = bytesLeidosReal;

    return bytesLeidosReal == cantidadDeBytes;
}

bool ProcesoExterno::EscribirMemoria(uintptr_t direccion, const void* buffer, SIZE_T cantidadDeBytes, SIZE_T* bytesEscritos) const
{
    if (!EstaAbierto() || direccion == 0 || buffer == nullptr || cantidadDeBytes == 0)
        return false;

    SIZE_T bytesEscritosReal = 0;
    if (!WriteProcessMemory(m_handle, reinterpret_cast<LPVOID>(direccion), buffer, cantidadDeBytes, &bytesEscritosReal))
    {
        m_ultimoError = GetLastError();
        return false;
    }

    if (bytesEscritos)
        *bytesEscritos = bytesEscritosReal;

    return bytesEscritosReal == cantidadDeBytes;
}
