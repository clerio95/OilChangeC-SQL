#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void trim_inplace(char* s) {
    size_t len;
    size_t start = 0;
    size_t end;

    if (s == NULL) {
        return;
    }

    len = strlen(s);
    if (len == 0) {
        return;
    }

    while (s[start] != '\0' && isspace((unsigned char)s[start])) {
        start++;
    }

    if (start > 0) {
        memmove(s, s + start, len - start + 1);
    }

    len = strlen(s);
    if (len == 0) {
        return;
    }

    end = len - 1;
    while (end > 0 && isspace((unsigned char)s[end])) {
        s[end] = '\0';
        end--;
    }
}

void normalizar_placa(char* placa) {
    size_t i;

    if (placa == NULL) {
        return;
    }

    trim_inplace(placa);
    for (i = 0; placa[i] != '\0'; i++) {
        placa[i] = (char)toupper((unsigned char)placa[i]);
    }
}

static int eh_mercosul(const char* p) {
    return (strlen(p) == 7 &&
            isalpha((unsigned char)p[0]) &&
            isalpha((unsigned char)p[1]) &&
            isalpha((unsigned char)p[2]) &&
            isdigit((unsigned char)p[3]) &&
            isalpha((unsigned char)p[4]) &&
            isdigit((unsigned char)p[5]) &&
            isdigit((unsigned char)p[6]));
}

static int eh_brasil_antigo(const char* p) {
    return (strlen(p) == 8 &&
            isalpha((unsigned char)p[0]) &&
            isalpha((unsigned char)p[1]) &&
            isalpha((unsigned char)p[2]) &&
            p[3] == '-' &&
            isdigit((unsigned char)p[4]) &&
            isdigit((unsigned char)p[5]) &&
            isdigit((unsigned char)p[6]) &&
            isdigit((unsigned char)p[7]));
}

int validar_placa(const char* placa) {
    char buf[32];

    if (placa == NULL) {
        return PLACA_INVALIDA;
    }

    snprintf(buf, sizeof(buf), "%s", placa);
    normalizar_placa(buf);

    if (eh_mercosul(buf)) {
        return PLACA_MERCOSUL;
    }

    if (eh_brasil_antigo(buf)) {
        return PLACA_BRASIL;
    }

    return PLACA_INVALIDA;
}

int validar_telefone(const char* telefone) {
    int digitos = 0;
    size_t i;

    if (telefone == NULL || telefone[0] == '\0') {
        return 0;
    }

    for (i = 0; telefone[i] != '\0'; i++) {
        if (isdigit((unsigned char)telefone[i])) {
            digitos++;
        } else if (telefone[i] != ' ' && telefone[i] != '(' && telefone[i] != ')' && telefone[i] != '-' && telefone[i] != '+') {
            return 0;
        }
    }

    return (digitos >= 10 && digitos <= 11) ? 1 : 0;
}

int validar_data(const char* data) {
    int y, m, d, hh, mm, ss;

    if (data == NULL) {
        return 0;
    }

    if (sscanf(data, "%4d-%2d-%2d %2d:%2d:%2d", &y, &m, &d, &hh, &mm, &ss) != 6) {
        return 0;
    }

    if (y < 2000 || y > 2100) return 0;
    if (m < 1 || m > 12) return 0;
    if (d < 1 || d > 31) return 0;
    if (hh < 0 || hh > 23) return 0;
    if (mm < 0 || mm > 59) return 0;
    if (ss < 0 || ss > 59) return 0;

    return 1;
}
