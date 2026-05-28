#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct
{
    char caminho_bd[MAX_PATH];    /* local DB path (primary) */
    char caminho_rede[MAX_PATH];  /* network sync path (optional replica) */
    char tema[20];
    int fonte_tamanho;
    int auto_backup;
    char pasta_backup[MAX_PATH];
} Config;

int config_carregar(const char *config_path, Config *config); /* 0=ok, 1=not found, -1=error */
int config_salvar(const char *config_path, const Config *config);
int config_abrir_dialogo(HWND hwndParent, Config *config, const char *config_path);

#endif
