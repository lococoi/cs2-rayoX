#include <Windows.h>

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
//  La dirección se obtiene de DOS formas:
//   1) Búsqueda de firma en la imagen de client.dll (automática,
//      sobrevive a parches de Valve mientras la función no cambie).
//   2) Offset fijo (DESPLAZAMIENTO_XRAY) como respaldo para el
//      build 14173.
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

// Busca la firma del X-Ray en la imagen completa de client.dll.
// Devuelve el offset dentro del módulo, o -1 si no la encuentra.
static intptr_t BuscarXrayPorFirma(ProcesoExterno& juego, uintptr_t baseDelModulo, SIZE_T tamanoDelModulo)
{
    vector<BYTE> imagenDelModulo(tamanoDelModulo);
    if (!juego.LeerMemoria(baseDelModulo, imagenDelModulo.data(), imagenDelModulo.size()))
        return -1;

    const BuscadorDeFirmas buscador(FIRMA_XRAY, TAMANO_DE_LA_FIRMA, MASCARA_FIRMA_XRAY);
    if (!buscador.EsFirmaValida())
        return -1;

    return buscador.BuscarEn(imagenDelModulo.data(), imagenDelModulo.size());
}

// Verifica que en la dirección haya el "xor al, al" esperado.
static bool LaDireccionContieneElXray(ProcesoExterno& juego, uintptr_t direccion)
{
    unsigned char byteActual = 0;
    if (!juego.LeerMemoria(direccion, &byteActual, sizeof(byteActual)))
        return false;
    return byteActual == 0x32; // primer byte de "xor al, al"
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

    // 3) Determinar la dirección del X-Ray: primero por firma, luego fallback.
    direccionDelXray = baseDelModulo + DESPLAZAMIENTO_XRAY;

    const intptr_t desplazamientoPorFirma = BuscarXrayPorFirma(juego, baseDelModulo, modulo.tamano);
    if (desplazamientoPorFirma != -1)
    {
        direccionDelXray = baseDelModulo + static_cast<uintptr_t>(desplazamientoPorFirma);
        cout << "[OK] X-Ray por firma: client.dll + 0x" << hex << desplazamientoPorFirma << dec << endl;
    }
    else
    {
        cout << "[WARN] Firma no encontrada; usando offset fijo 0x"
             << hex << DESPLAZAMIENTO_XRAY << dec << endl;
    }

    // 4) Verificar la dirección antes de usarla.
    if (!LaDireccionContieneElXray(juego, direccionDelXray))
    {
        cerr << "[ERROR] La dirección no contiene el patrón esperado (0x32). "
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
