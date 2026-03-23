#include "config.h"

#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void config_defaults(Config* config) {
    if (config == NULL) {
        return;
    }

    snprintf(config->caminho_bd, sizeof(config->caminho_bd), "C:\\TrocaOleo\\dados.db");
    snprintf(config->tema, sizeof(config->tema), "claro");
    config->fonte_tamanho = 10;
    config->auto_backup = 1;
    snprintf(config->pasta_backup, sizeof(config->pasta_backup), "C:\\TrocaOleo\\Backups");
}

static void trim_line(char* line) {
    size_t len;
    if (line == NULL) {
        return;
    }

    len = strlen(line);
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || line[len - 1] == ' ' || line[len - 1] == '\t')) {
        line[len - 1] = '\0';
        len--;
    }
}

int config_carregar(const char* config_path, Config* config) {
    FILE* f;
    char line[512];

    if (config == NULL || config_path == NULL) {
        return -1;
    }

    config_defaults(config);

    f = fopen(config_path, "r");
    if (f == NULL) {
        return config_salvar(config_path, config);
    }

    while (fgets(line, sizeof(line), f) != NULL) {
        trim_line(line);

        if (strncmp(line, "caminho=", 8) == 0) {
            snprintf(config->caminho_bd, sizeof(config->caminho_bd), "%s", line + 8);
        } else if (strncmp(line, "tema=", 5) == 0) {
            snprintf(config->tema, sizeof(config->tema), "%s", line + 5);
        } else if (strncmp(line, "fonte_tamanho=", 14) == 0) {
            config->fonte_tamanho = atoi(line + 14);
        } else if (strncmp(line, "auto_backup=", 12) == 0) {
            config->auto_backup = atoi(line + 12);
        } else if (strncmp(line, "pasta_backup=", 13) == 0) {
            snprintf(config->pasta_backup, sizeof(config->pasta_backup), "%s", line + 13);
        }
    }

    fclose(f);
    return 0;
}

int config_salvar(const char* config_path, const Config* config) {
    FILE* f;

    if (config == NULL || config_path == NULL) {
        return -1;
    }

    f = fopen(config_path, "w");
    if (f == NULL) {
        return -1;
    }

    fprintf(f, "[Database]\n");
    fprintf(f, "caminho=%s\n\n", config->caminho_bd);

    fprintf(f, "[Interface]\n");
    fprintf(f, "tema=%s\n", config->tema);
    fprintf(f, "fonte_tamanho=%d\n\n", config->fonte_tamanho);

    fprintf(f, "[Backup]\n");
    fprintf(f, "auto_backup=%d\n", config->auto_backup);
    fprintf(f, "pasta_backup=%s\n", config->pasta_backup);

    fclose(f);
    return 0;
}

void config_abrir_dialogo(HWND hwndParent, Config* config, const char* config_path) {
    OPENFILENAME ofn;
    char fileName[MAX_PATH];

    if (config == NULL || config_path == NULL) {
        return;
    }

    ZeroMemory(&ofn, sizeof(ofn));
    ZeroMemory(fileName, sizeof(fileName));
    snprintf(fileName, sizeof(fileName), "%s", config->caminho_bd);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndParent;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "SQLite Database (*.db)\0*.db\0All Files (*.*)\0*.*\0";
    ofn.lpstrDefExt = "db";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        snprintf(config->caminho_bd, sizeof(config->caminho_bd), "%s", fileName);
        if (config_salvar(config_path, config) == 0) {
            MessageBox(hwndParent,
                       "Caminho do banco atualizado. Reinicie o aplicativo para reconectar.",
                       "Configuracoes",
                       MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hwndParent,
                       "Nao foi possivel salvar o arquivo config.ini.",
                       "Erro",
                       MB_OK | MB_ICONERROR);
        }
    }
}
