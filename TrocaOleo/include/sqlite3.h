#ifndef SQLITE3_FORWARD_HEADER_H
#define SQLITE3_FORWARD_HEADER_H

/*
 * Wrapper para permitir manter a estrutura include/sqlite3.h no projeto.
 * Em MinGW/GCC, include_next busca o sqlite3.h real instalado no sistema.
 */
#if defined(__GNUC__)
#include_next <sqlite3.h>
#else
#include <sqlite3.h>
#endif

#endif
