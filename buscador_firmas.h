#pragma once

#include <Windows.h>

// ============================================================
//  BuscadorDeFirmas: localiza una firma de bytes (patrón) dentro
//  de un buffer de memoria.
//  Soporta comodines '?' en la máscara (el byte se ignora).
//  La firma puede contener bytes nulos (\x00) internos; su
//  longitud real la define la máscara.
//  lococoi
// ============================================================
class BuscadorDeFirmas
{
public:
    // 'firma' debe tener 'tamanoDeLaFirma' bytes y 'mascara' una longitud
    // strlen() igual a 'tamanoDeLaFirma'.
    BuscadorDeFirmas(const char* firma, SIZE_T tamanoDeLaFirma, const char* mascara);
    ~BuscadorDeFirmas();

    // No copiable (posee buffers internos).
    BuscadorDeFirmas(const BuscadorDeFirmas&) = delete;
    BuscadorDeFirmas& operator=(const BuscadorDeFirmas&) = delete;

    // false si los argumentos eran inválidos (null, longitud 0 o distinta).
    bool EsFirmaValida() const { return m_esValida; }

    // Devuelve el offset del primer match dentro de 'buffer',
    // o -1 si la firma no se encuentra o el buffer es muy chico.
    intptr_t BuscarEn(const BYTE* buffer, SIZE_T tamanoDelBuffer) const;

private:
    char*  m_firma;    // copia de los bytes de la firma
    char*  m_mascara;  // copia de la máscara
    SIZE_T m_longitud;
    bool   m_esValida;
};
