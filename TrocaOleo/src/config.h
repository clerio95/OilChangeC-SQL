#ifndef CONFIG_H
#define CONFIG_H

#include <windows.h>

typedef struct {
    char caminho_bd[MAX_PATH];
    char tema[20];
    int fonte_tamanho;
    int auto_backup;
    char pasta_backup[MAX_PATH];
} Config;

int config_carregar(const char* config_path, Config* config);
int config_salvar(const char* config_path, const Config* config);
void config_abrir_dialogo(HWND hwndParent, Config* config, const char* config_path);

#endif
