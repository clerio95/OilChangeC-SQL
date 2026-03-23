#ifndef VALIDATION_H
#define VALIDATION_H

typedef enum {
    PLACA_INVALIDA = 0,
    PLACA_MERCOSUL = 1,
    PLACA_BRASIL = 2
} FormatoPlaca;

int validar_placa(const char* placa);
int validar_telefone(const char* telefone);
int validar_data(const char* data);
void normalizar_placa(char* placa);

#endif
