#ifndef DATABASE_H
#define DATABASE_H

#include <stddef.h>
#include "../include/sqlite3.h"

typedef struct {
    int id;
    char placa[10];
    char tipo_oleo[50];
    char telefone[20];
    int telefone_informado;
    int veio_indicacao;
    char data_troca[20];
    char data_cadastro[20];
    int ativo;
} TrocaOleo;

typedef struct {
    int id;
    char nome[50];
    int ativo;
} TipoOleo;

int db_init(const char* db_path);
int db_criar_tabelas(void);

int db_inserir_troca(const TrocaOleo* troca);
TrocaOleo* db_listar_trocas(const char* filtro_placa, int* count);
TrocaOleo* db_listar_ultimas_trocas(const char* filtro_placa, int* count);
int db_atualizar_troca(int id, const TrocaOleo* troca);
int db_deletar_troca(int id);
TrocaOleo* db_buscar_troca_por_id(int id);

TrocaOleo* db_historico_por_placa(const char* placa, int* count);
int db_contar_trocas_por_placa(const char* placa);
char* db_tipo_oleo_mais_usado(const char* placa);
int db_intervalo_medio_dias(const char* placa);
char* db_data_primeira_troca(const char* placa);
char* db_data_ultima_troca(const char* placa);

int db_total_veiculos_cadastrados(void);
int db_total_trocas_realizadas(void);
char** db_listar_placas_unicas(int* count);

TipoOleo* db_listar_tipos_oleo(int* count);
int db_adicionar_tipo_oleo(const char* nome);
int db_remover_tipo_oleo(int id);

void db_fechar(void);
void db_liberar_trocas(TrocaOleo* trocas);
void db_liberar_tipos(TipoOleo* tipos);
void db_liberar_lista_strings(char** lista, int count);

#endif
