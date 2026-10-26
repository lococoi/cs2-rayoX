#include <Windows.h>

#include <cstring>
#include <iostream>
#include <vector>

#include "buscador_firmas.h"
#include "config.h"
#include "gestor_modulos.h"
#include "proceso_externo.h"

using namespace std;

// ============================================================
//  cs2-rayoX — X-RAY por parcheo directo de la función.
//  En la dirección del X-Ray hay un "xor al, al" (32 C0).
//  Escribir "mov al, 1" (B0 01) activa el glow en todos los
//  jugadores al instante; restaurar "xor al, al" lo apaga.
//
//  La dirección se obtiene de TRES formas:
//   1) Acceso al flag + salto relativo (cmp [rax+58],0; je) en la
//      imagen de client.dll; no depende del epílogo.
//   2) Búsqueda del epílogo del X-Ray como respaldo.
//   3) Offset fijo (DESPLAZAMIENTO_XRAY) como último recurso.
//
//  Medidas de discreción:
//   - Al salir (Ctrl+C / cierre) se restauran los bytes originales.
//   - Antes de escribir se verifica que la dirección tenga el byte
//     esperado (0x32), para nunca modificar memoria equivocada.
//   - La consola puede ocultarse con OCULTAR_CONSOLA en config.h.
//
//  lococoi
// ============================================================

// Estado global para poder restaurar los bytes al salir.
static ProcesoExterno* procesoDelJuego  = nullptr;
static uintptr_t       direccionDelXray = 0;
static bool            xrayActivado     = false;

static BOOL WINAPI RestaurarBytesAlSalir(DWORD tipoDeEvento)
{
    if ((tipoDeEvento == CTRL_C_EVENT || tipoDeEvento == CTRL_CLOSE_EVENT) &&
        procesoDelJuego != nullptr && direccionDelXray != 0 && xrayActivado)
    {
        procesoDelJuego->EscribirMemoria(direccionDelXray,
                                         BYTES_XRAY_APAGADO, sizeof(BYTES_XRAY_APAGADO));
        xrayActivado = false;
    }
    return FALSE;
}

// Devuelve true si en 'offset' empieza el epílogo del X-Ray.
static bool CoincideEpilogoXray(const BYTE* imagen, SIZE_T tamano, intptr_t offset)
{
    if (offset < 0 || static_cast<SIZE_T>(offset) + TAMANO_DE_LA_FIRMA > tamano)
        return false;

    return memcmp(imagen + offset, FIRMA_XRAY, TAMANO_DE_LA_FIRMA) == 0;
}

// Etapa 1: busca el acceso al flag (cmp [rax+58], 0; je rel32) y
// sigue el salto relativo hasta el "xor al, al" objetivo.
// Devuelve el offset del objetivo, o -1 si no hay un candidato claro.
static intptr_t BuscarXrayPorAcceso(const BYTE* imagen, SIZE_T tamano)
{
    const BuscadorDeFirmas buscador(FIRMA_ACCESO_XRAY, TAMANO_FIRMA_ACCESO, MASCARA_ACCESO_XRAY);
    if (!buscador.EsFirmaValida())
        return -1;

    vector<intptr_t> candidatos;
    SIZE_T desde = 0;

    while (desde + TAMANO_FIRMA_ACCESO <= tamano)
    {
        const intptr_t relativo = buscador.BuscarEn(imagen + desde, tamano - desde);
        if (relativo < 0)
            break;

        const SIZE_T posicion = desde + static_cast<SIZE_T>(relativo);

        // El rel32 está a 6 bytes del inicio del patrón (después del cmp de 4
        // bytes y del 0F 84). La instrucción siguiente al je está a 10 bytes.
        int32_t desplazamiento = 0;
        memcpy(&desplazamiento, imagen + posicion + 6, sizeof(desplazamiento));

        const intptr_t objetivo = static_cast<intptr_t>(posicion) + 10 + desplazamiento;

        if (objetivo > static_cast<intptr_t>(posicion) &&
            static_cast<SIZE_T>(objetivo) + 2 <= tamano &&
            (imagen[objetivo] == 0x32 || imagen[objetivo] == 0x30) &&
            imagen[objetivo + 1] == 0xC0)
        {
            candidatos.push_back(objetivo);
        }

        desde = posicion + 1;
    }

    if (candidatos.size() == 1)
        return candidatos[0];

    // Si hay varios accesos, el epílogo desambigua.
    if (candidatos.size() > 1)
    {
        intptr_t elegido = -1;
        int coincidencias = 0;

        for (const intptr_t candidato : candidatos)
        {
            if (CoincideEpilogoXray(imagen, tamano, candidato))
            {
                elegido = candidato;
                coincidencias++;
            }
        }

        if (coincidencias == 1)
            return elegido;
    }

    return -1;
}

// Etapa 2: busca directamente el epílogo del X-Ray.
// Solo devuelve un resultado si la coincidencia es única.
static intptr_t BuscarXrayPorEpilogo(const BYTE* imagen, SIZE_T tamano)
{
    const BuscadorDeFirmas buscador(FIRMA_XRAY, TAMANO_DE_LA_FIRMA, MASCARA_FIRMA_XRAY);
    if (!buscador.EsFirmaValida())
        return -1;

    intptr_t encontrado = -1;
    int coincidencias = 0;
    SIZE_T desde = 0;

    while (desde + TAMANO_DE_LA_FIRMA <= tamano)
    {
        const intptr_t relativo = buscador.BuscarEn(imagen + desde, tamano - desde);
        if (relativo < 0)
            break;

        const SIZE_T posicion = desde + static_cast<SIZE_T>(relativo);
        encontrado = static_cast<intptr_t>(posicion);
        coincidencias++;
        desde = posicion + 1;
    }

    return coincidencias == 1 ? encontrado : -1;
}

// Lee la imagen del módulo y aplica las etapas en orden.
// 'metodo' recibe el nombre de la etapa que encontró la dirección.
static intptr_t BuscarXray(ProcesoExterno& juego, uintptr_t baseDelModulo, SIZE_T tamanoDelModulo,
                           const char** metodo)
{
    vector<BYTE> imagenDelModulo(tamanoDelModulo);
    if (!juego.LeerMemoria(baseDelModulo, imagenDelModulo.data(), imagenDelModulo.size()))
        return -1;

    const intptr_t porAcceso  = BuscarXrayPorAcceso(imagenDelModulo.data(), imagenDelModulo.size());
    const intptr_t porEpilogo = BuscarXrayPorEpilogo(imagenDelModulo.data(), imagenDelModulo.size());

    // Si ambos métodos coinciden, es la respuesta con más confianza.
    if (porAcceso != -1 && porEpilogo != -1)
    {
        if (porAcceso == porEpilogo)
        {
            *metodo = "acceso + epílogo (coinciden)";
            return porAcceso;
        }

        // Si no coinciden, el epílogo (22 bytes) es más específico.
        *metodo = "epílogo (el acceso no coincide)";
        return porEpilogo;
    }

    if (porAcceso != -1)
    {
        *metodo = "acceso (cmp+je)";
        return porAcceso;
    }

    if (porEpilogo != -1)
    {
        *metodo = "epílogo";
        return porEpilogo;
    }

    *metodo = nullptr;
    return -1;
}

// Verifica que en la dirección haya el "xor al, al" esperado (32 C0 o 30 C0).
static bool LaDireccionContieneElXray(ProcesoExterno& juego, uintptr_t direccion)
{
    unsigned char bytes[2] = {};
    if (!juego.LeerMemoria(direccion, bytes, sizeof(bytes)))
        return false;
    return (bytes[0] == 0x32 || bytes[0] == 0x30) && bytes[1] == 0xC0;
}

int main()
{
    SetConsoleCtrlHandler(RestaurarBytesAlSalir, TRUE);

    if (OCULTAR_CONSOLA)
        ShowWindow(GetConsoleWindow(), SW_HIDE);

    cout << "cs2-rayoX — X-Ray para Counter-Strike 2" << endl;
    cout << "hecho por lococoi" << endl;
    cout << "Esperando proceso: " << NOMBRE_DEL_JUEGO << " ..." << endl;

    // 1) Esperar a que el juego esté abierto (reintenta cada segundo).
    ProcesoExterno juego;
    while (!juego.AbrirProceso(NOMBRE_DEL_JUEGO, NIVEL_DE_ACCESO))
    {
        const DWORD codigoDeError = juego.ObtenerCodigoDeError();
        if (codigoDeError == ERROR_ACCESS_DENIED)
        {
            cerr << "[ERROR] Acceso denegado. Ejecutá como Administrador." << endl;
            return 1;
        }
        Sleep(INTERVALO_DE_REINTENTO_MS);
    }
    procesoDelJuego = &juego;
    cout << "[OK] Proceso encontrado. PID: " << juego.ObtenerPid() << endl;

    // 2) Obtener el módulo client.dll.
    InfoModulo modulo;
    if (!GestorDeModulos::ObtenerModulo(juego.ObtenerPid(), NOMBRE_DEL_MODULO, modulo))
    {
        cerr << "[ERROR] No se encontró el módulo client.dll." << endl;
        return 1;
    }
    const uintptr_t baseDelModulo = modulo.direccionBase;
    cout << "[OK] client.dll: 0x" << hex << baseDelModulo << dec
         << " (" << modulo.tamano << " bytes)" << endl;

    // 3) Determinar la dirección del X-Ray: acceso+salto, epílogo y fallback.
    direccionDelXray = baseDelModulo + DESPLAZAMIENTO_XRAY;

    const char* metodo = nullptr;
    const intptr_t desplazamientoEncontrado = BuscarXray(juego, baseDelModulo, modulo.tamano, &metodo);
    if (desplazamientoEncontrado != -1)
    {
        direccionDelXray = baseDelModulo + static_cast<uintptr_t>(desplazamientoEncontrado);
        cout << "[OK] X-Ray por " << metodo << ": client.dll + 0x"
             << hex << desplazamientoEncontrado << dec << endl;
    }
    else
    {
        cout << "[WARN] Firma no encontrada; usando offset fijo 0x"
             << hex << DESPLAZAMIENTO_XRAY << dec << endl;
    }

    // 4) Verificar la dirección antes de usarla.
    if (!LaDireccionContieneElXray(juego, direccionDelXray))
    {
        cerr << "[ERROR] La dirección no contiene el patrón esperado (32 C0 / 30 C0). "
                "Actualizá la firma o el offset en config.h." << endl;
        return 1;
    }

    // 5) Loop principal: activar/desactivar con la tecla (flanco de subida).
    bool teclaAnterior = false;

    cout << "F1 para activar/desactivar el X-Ray." << endl;

    while (true)
    {
        const bool teclaPresionada = (GetAsyncKeyState(TECLA_ACTIVAR_XRAY) & 0x8000) != 0;

        // Solo alterna en la pulsación, no al mantener la tecla.
        if (teclaPresionada && !teclaAnterior)
        {
            xrayActivado = !xrayActivado;

            const unsigned char* bytesXray    = xrayActivado ? BYTES_XRAY_ENCENDIDO : BYTES_XRAY_APAGADO;
            const SIZE_T tamanoDeLosBytes     = xrayActivado ? sizeof(BYTES_XRAY_ENCENDIDO)
                                                             : sizeof(BYTES_XRAY_APAGADO);

            if (!juego.EscribirMemoria(direccionDelXray, bytesXray, tamanoDeLosBytes))
            {
                cerr << "[ERROR] No se pudo escribir (error "
                     << juego.ObtenerCodigoDeError() << "). ¿Se cerró el juego?" << endl;
                return 1;
            }

            cout << (xrayActivado ? "[X-RAY] ACTIVADO" : "[X-RAY] desactivado") << endl;
        }

        teclaAnterior = teclaPresionada;
        Sleep(INTERVALO_DEL_LOOP_MS);
    }

    return 0;
}
