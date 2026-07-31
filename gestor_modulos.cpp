#include "gestor_modulos.h"

#include <TlHelp32.h>
#include <cstring>

bool GestorDeModulos::ObtenerModulo(DWORD pid, const wchar_t* nombreDelModulo, InfoModulo& resultado)
{
    resultado = InfoModulo{};

    HANDLE capturaDeModulos = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (capturaDeModulos == INVALID_HANDLE_VALUE)
        return false;

    bool encontrado = false;
    MODULEENTRY32W entradaDeModulo{};
    entradaDeModulo.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(capturaDeModulos, &entradaDeModulo))
    {
        // do-while: el primer módulo del snapshot también se compara.
        do
        {
            if (_wcsicmp(entradaDeModulo.szModule, nombreDelModulo) == 0)
            {
                resultado.direccionBase = reinterpret_cast<uintptr_t>(entradaDeModulo.modBaseAddr);
                resultado.tamano        = entradaDeModulo.modBaseSize;
                encontrado = true;
                break;
            }
        } while (Module32NextW(capturaDeModulos, &entradaDeModulo));
    }

    CloseHandle(capturaDeModulos);
    return encontrado;
}
