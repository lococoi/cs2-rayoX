#include "buscador_firmas.h"

#include <cstring>
#include <new>

BuscadorDeFirmas::BuscadorDeFirmas(const char* firma, SIZE_T tamanoDeLaFirma, const char* mascara)
    : m_firma(nullptr), m_mascara(nullptr), m_longitud(0), m_esValida(false)
{
    if (firma == nullptr || mascara == nullptr || tamanoDeLaFirma == 0)
        return;

    // La firma puede contener bytes nulos internos; la longitud real la
    // aporta el llamador y debe coincidir con la de la máscara.
    const SIZE_T tamanoDeLaMascara = strlen(mascara);
    if (tamanoDeLaMascara != tamanoDeLaFirma)
        return;

    m_longitud = tamanoDeLaFirma;
    m_firma  = new (std::nothrow) char[m_longitud];
    m_mascara = new (std::nothrow) char[m_longitud + 1];

    if (m_firma == nullptr || m_mascara == nullptr)
    {
        delete[] m_firma;
        delete[] m_mascara;
        m_firma   = nullptr;
        m_mascara = nullptr;
        m_longitud = 0;
        return;
    }

    memcpy(m_firma, firma, m_longitud);
    memcpy(m_mascara, mascara, m_longitud + 1);
    m_esValida = true;
}

BuscadorDeFirmas::~BuscadorDeFirmas()
{
    delete[] m_firma;
    delete[] m_mascara;
}

intptr_t BuscadorDeFirmas::BuscarEn(const BYTE* buffer, SIZE_T tamanoDelBuffer) const
{
    if (!m_esValida || buffer == nullptr || tamanoDelBuffer < m_longitud)
        return -1;

    for (SIZE_T i = 0; i <= tamanoDelBuffer - m_longitud; i++)
    {
        bool encontrado = true;

        for (SIZE_T j = 0; j < m_longitud; j++)
        {
            if (m_mascara[j] != '?' && static_cast<BYTE>(m_firma[j]) != buffer[i + j])
            {
                encontrado = false;
                break;
            }
        }

        if (encontrado)
            return static_cast<intptr_t>(i);
    }

    return -1;
}
