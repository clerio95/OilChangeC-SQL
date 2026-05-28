#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "database.h"
#include "gui.h"
#include "validation.h"

static Config g_config;
static int g_edit_id = -1;
static char g_config_path[MAX_PATH];
static int g_formatando_telefone = 0;
static int g_startup_pull_rc = 0;

static void montar_caminho_config(char *out, size_t out_size)
{
    size_t i;

    if (out == NULL || out_size == 0)
    {
        return;
    }

    if (GetModuleFileNameA(NULL, out, (DWORD)out_size) == 0)
    {
        snprintf(out, out_size, "config.ini");
        return;
    }

    for (i = strlen(out); i > 0; i--)
    {
        if (out[i] == '\\' || out[i] == '/')
        {
            out[i + 1] = '\0';
            break;
        }
    }

    strncat(out, "config.ini", out_size - strlen(out) - 1);
}

/* Builds the path to dados.db in the same folder as the exe. */
static void montar_caminho_local_db(char *out, size_t out_size)
{
    size_t i;

    if (out == NULL || out_size == 0)
        return;

    if (GetModuleFileNameA(NULL, out, (DWORD)out_size) == 0)
    {
        snprintf(out, out_size, "dados.db");
        return;
    }

    for (i = strlen(out); i > 0; i--)
    {
        if (out[i] == '\\' || out[i] == '/')
        {
            out[i + 1] = '\0';
            break;
        }
    }

    strncat(out, "dados.db", out_size - strlen(out) - 1);
}

static void hora_atual(char *out, int size)
{
    SYSTEMTIME st;
    GetLocalTime(&st);
    snprintf(out, size, "%02d:%02d", st.wHour, st.wMinute);
}

/* Pushes local DB to network and updates the status bar. */
static void sincronizar_rede_silencioso(HWND hwnd)
{
    char msg[64];
    char hora[8];

    if (g_config.caminho_rede[0] == '\0')
        return;

    hora_atual(hora, sizeof(hora));
    if (db_sincronizar_para_rede(g_config.caminho_rede) == 0)
        snprintf(msg, sizeof(msg), "Sincronizado com a rede — %s", hora);
    else
        snprintf(msg, sizeof(msg), "Aviso: falha ao sincronizar com a rede");

    atualizar_status(hwnd, msg);
}

static void aplicar_mascara_telefone(HWND hwndEdit)
{
    char atual[32];
    char digitos[16];
    char formatado[32];
    int i;
    int n = 0;
    int pos = 0;

    if (g_formatando_telefone)
    {
        return;
    }

    GetWindowText(hwndEdit, atual, (int)sizeof(atual));

    for (i = 0; atual[i] != '\0' && n < 11; i++)
    {
        if (isdigit((unsigned char)atual[i]))
        {
            digitos[n++] = atual[i];
        }
    }
    digitos[n] = '\0';

    formatado[0] = '\0';
    if (n > 0)
    {
        formatado[pos++] = '(';
    }
    if (n > 0)
    {
        formatado[pos++] = digitos[0];
    }
    if (n > 1)
    {
        formatado[pos++] = digitos[1];
        formatado[pos++] = ')';
    }
    for (i = 2; i < n && i < 7; i++)
    {
        formatado[pos++] = digitos[i];
    }
    if (n > 7)
    {
        formatado[pos++] = '-';
        for (i = 7; i < n && i < 11; i++)
        {
            formatado[pos++] = digitos[i];
        }
    }
    formatado[pos] = '\0';

    g_formatando_telefone = 1;
    SetWindowText(hwndEdit, formatado);
    SendMessage(hwndEdit, EM_SETSEL, (WPARAM)strlen(formatado), (LPARAM)strlen(formatado));
    g_formatando_telefone = 0;
}

static void trim_texto(char *s)
{
    size_t len;
    size_t ini = 0;

    if (s == NULL)
    {
        return;
    }

    while (s[ini] != '\0' && isspace((unsigned char)s[ini]))
    {
        ini++;
    }

    if (ini > 0)
    {
        memmove(s, s + ini, strlen(s + ini) + 1);
    }

    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[len - 1] = '\0';
        len--;
    }
}

static int criar_diretorios_para_arquivo(const char *caminho_arquivo)
{
    wchar_t wpasta[MAX_PATH];
    size_t i;
    size_t inicio;
    int separadores;
    int len;

    if (caminho_arquivo == NULL || caminho_arquivo[0] == '\0')
    {
        return -1;
    }

    /* Converter ANSI para wide char para suportar acentos em caminhos de rede */
    len = MultiByteToWideChar(CP_ACP, 0, caminho_arquivo, -1, wpasta, MAX_PATH);
    if (len == 0)
    {
        return -1;
    }

    /* Remover nome do arquivo, manter so a pasta */
    for (i = wcslen(wpasta); i > 0; i--)
    {
        if (wpasta[i] == L'\\' || wpasta[i] == L'/')
        {
            wpasta[i] = L'\0';
            break;
        }
    }

    if (wpasta[0] == L'\0')
    {
        return 0;
    }

    /* Caminho UNC (\\servidor\compartilhamento\...): pular servidor e share */
    if (wpasta[0] == L'\\' && wpasta[1] == L'\\')
    {
        separadores = 0;
        for (inicio = 2; wpasta[inicio] != L'\0'; inicio++)
        {
            if (wpasta[inicio] == L'\\' || wpasta[inicio] == L'/')
            {
                separadores++;
                if (separadores == 2)
                {
                    inicio++;
                    break;
                }
            }
        }
    }
    else
    {
        /* Caminho local (ex: C:\...) */
        inicio = 3;
    }

    for (i = inicio; wpasta[i] != L'\0'; i++)
    {
        if (wpasta[i] == L'\\' || wpasta[i] == L'/')
        {
            wchar_t tmp = wpasta[i];
            wpasta[i] = L'\0';
            CreateDirectoryW(wpasta, NULL);
            wpasta[i] = tmp;
        }
    }

    CreateDirectoryW(wpasta, NULL);
    return 0;
}

static int validar_dados_formulario(HWND hwnd, TrocaOleo *t)
{
    int f;

    normalizar_placa(t->placa);

    f = validar_placa(t->placa);
    if (f == PLACA_INVALIDA)
    {
        mostrar_erro(hwnd,
                     "Placa invalida.\n\n"
                     "Formatos aceitos:\n"
                     "- Mercosul: ABC1D23\n"
                     "- Brasil: ABC-1234");
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_PLACA));
        return 0;
    }

    if (t->tipo_oleo[0] == '\0')
    {
        mostrar_erro(hwnd, "Selecione um tipo de oleo.");
        return 0;
    }

    if (t->telefone_informado && !validar_telefone(t->telefone))
    {
        mostrar_erro(hwnd, "Telefone invalido. Formato esperado: (XX)XXXXX-XXXX.");
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_TELEFONE));
        return 0;
    }

    if (!validar_data(t->data_troca))
    {
        mostrar_erro(hwnd, "Data da troca invalida.");
        return 0;
    }

    return 1;
}

static void carregar_lista_principal(HWND hwnd)
{
    char filtro[32];
    int usar_ultimas;
    int count = 0;
    TrocaOleo *lista;
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_BUSCA), filtro, (int)sizeof(filtro));

    usar_ultimas = (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_EXIBIR_ULTIMA)) == BST_CHECKED);

    if (usar_ultimas)
    {
        lista = db_listar_ultimas_trocas(filtro, &count);
    }
    else
    {
        lista = db_listar_trocas(filtro, &count);
    }

    if (lista == NULL && count == 0)
    {
        ListView_DeleteAllItems(hList);
        return;
    }

    atualizar_listview(hList, lista, count);
    db_liberar_trocas(lista);
}

static void acao_salvar(HWND hwnd)
{
    TrocaOleo t = obter_dados_formulario(hwnd);

    if (!validar_dados_formulario(hwnd, &t))
    {
        return;
    }

    if (db_inserir_troca(&t) != 0)
    {
        mostrar_erro(hwnd, "Falha ao salvar no banco de dados.");
        return;
    }

    sincronizar_rede_silencioso(hwnd);
    carregar_lista_principal(hwnd);
    limpar_formulario(hwnd);
    mostrar_sucesso(hwnd, "Troca cadastrada com sucesso.");
}

static void acao_editar(HWND hwnd)
{
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
    TrocaOleo *t = obter_item_selecionado(hList);

    if (t == NULL)
    {
        mostrar_erro(hwnd, "Selecione um registro para editar.");
        return;
    }

    g_edit_id = t->id;
    preencher_formulario(hwnd, t);
    db_liberar_trocas(t);
}

static void acao_atualizar(HWND hwnd)
{
    TrocaOleo t = obter_dados_formulario(hwnd);

    if (g_edit_id <= 0)
    {
        mostrar_erro(hwnd, "Nenhum registro selecionado para atualizacao.");
        return;
    }

    if (!validar_dados_formulario(hwnd, &t))
    {
        return;
    }

    if (db_atualizar_troca(g_edit_id, &t) != 0)
    {
        mostrar_erro(hwnd, "Falha ao atualizar registro.");
        return;
    }

    g_edit_id = -1;
    sincronizar_rede_silencioso(hwnd);
    carregar_lista_principal(hwnd);
    limpar_formulario(hwnd);
    mostrar_sucesso(hwnd, "Registro atualizado com sucesso.");
}

static void acao_deletar(HWND hwnd)
{
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
    int id = obter_id_item_selecionado(hList);

    if (id < 0)
    {
        mostrar_erro(hwnd, "Selecione um registro para excluir.");
        return;
    }

    if (!confirmar_acao(hwnd, "Confirmar exclusao (soft delete) do registro selecionado?"))
    {
        return;
    }

    if (db_deletar_troca(id) != 0)
    {
        mostrar_erro(hwnd, "Falha ao excluir registro.");
        return;
    }

    sincronizar_rede_silencioso(hwnd);
    carregar_lista_principal(hwnd);
    mostrar_sucesso(hwnd, "Registro excluido com sucesso.");
}

static void acao_ver_historico_por_placa(HWND hwnd)
{
    char placa[16];

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_PLACA), placa, (int)sizeof(placa));
    normalizar_placa(placa);

    if (placa[0] == '\0')
    {
        HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
        TrocaOleo *t = obter_item_selecionado(hList);
        if (t != NULL)
        {
            snprintf(placa, sizeof(placa), "%s", t->placa);
            db_liberar_trocas(t);
        }
    }

    abrir_janela_historico(hwnd, placa);
}

static void acao_adicionar_tipo_oleo(HWND hwnd)
{
    char nome[50];

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_NOVO_OLEO), nome, (int)sizeof(nome));
    trim_texto(nome);

    if (nome[0] == '\0')
    {
        mostrar_erro(hwnd, "Informe um nome para o novo tipo de oleo.");
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_NOVO_OLEO));
        return;
    }

    if (db_adicionar_tipo_oleo(nome) != 0)
    {
        mostrar_erro(hwnd, "Nao foi possivel cadastrar o tipo de oleo (pode ja existir). ");
        return;
    }

    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_NOVO_OLEO), "");
    recarregar_tipos_oleo(hwnd);
    mostrar_sucesso(hwnd, "Novo tipo de oleo cadastrado com sucesso.");
}

static void acao_remover_tipo_oleo(HWND hwnd)
{
    char nome[50];
    int count = 0;
    TipoOleo *tipos;

    if (!obter_nome_tipo_oleo_selecionado(nome, (int)sizeof(nome)))
    {
        mostrar_erro(hwnd, "Selecione um tipo de oleo para remover.");
        return;
    }

    tipos = db_listar_tipos_oleo(&count);
    db_liberar_tipos(tipos);

    if (count <= 1)
    {
        mostrar_erro(hwnd, "Nao e permitido remover o ultimo tipo de oleo ativo.");
        return;
    }

    if (!confirmar_acao(hwnd, "Remover o tipo de oleo selecionado?"))
    {
        return;
    }

    if (db_remover_tipo_oleo_por_nome(nome) != 0)
    {
        mostrar_erro(hwnd, "Nao foi possivel remover o tipo de oleo selecionado.");
        return;
    }

    recarregar_tipos_oleo(hwnd);
    mostrar_sucesso(hwnd, "Tipo de oleo removido com sucesso.");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        criar_controles(hwnd);
        recarregar_tipos_oleo(hwnd);
        carregar_lista_principal(hwnd);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_SALVAR:
            acao_salvar(hwnd);
            break;

        case IDC_BUTTON_EDITAR:
            acao_editar(hwnd);
            break;

        case IDC_BUTTON_ATUALIZAR:
            acao_atualizar(hwnd);
            break;

        case IDC_BUTTON_LIMPAR:
            g_edit_id = -1;
            limpar_formulario(hwnd);
            break;

        case IDC_BUTTON_DELETAR:
            acao_deletar(hwnd);
            break;

        case IDC_BUTTON_PESQUISAR:
            carregar_lista_principal(hwnd);
            break;

        case IDC_RADIO_EXIBIR_TODAS:
        case IDC_RADIO_EXIBIR_ULTIMA:
            carregar_lista_principal(hwnd);
            break;

        case IDC_CHECK_TELEFONE:
        {
            int marcado = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_TELEFONE)) == BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), marcado);
            if (!marcado)
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), "");
            break;
        }

        case IDC_CHECK_KM_SEMANAL:
        {
            int marcado = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_KM_SEMANAL)) == BST_CHECKED);
            EnableWindow(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), marcado);
            if (!marcado)
                SetWindowText(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), "");
            break;
        }

        case IDC_EDIT_TELEFONE:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                aplicar_mascara_telefone(GetDlgItem(hwnd, IDC_EDIT_TELEFONE));
            }
            break;

        case IDC_BUTTON_VER_HISTORICO:
        case IDC_BUTTON_VER_HISTORICO_COMPLETO:
            acao_ver_historico_por_placa(hwnd);
            break;

        case IDC_BUTTON_ADICIONAR_OLEO:
            acao_adicionar_tipo_oleo(hwnd);
            break;

        case IDC_BUTTON_REMOVER_OLEO:
            acao_remover_tipo_oleo(hwnd);
            break;

        case IDM_CONFIG_BD:
        {
            char caminho_anterior[MAX_PATH];

            snprintf(caminho_anterior, sizeof(caminho_anterior), "%s", g_config.caminho_bd);

            if (config_abrir_dialogo(hwnd, &g_config, g_config_path))
            {
                db_fechar();
                criar_diretorios_para_arquivo(g_config.caminho_bd);
                if (db_init(g_config.caminho_bd, 1) != 0 || db_criar_tabelas() != 0)
                {
                    snprintf(g_config.caminho_bd, sizeof(g_config.caminho_bd), "%s", caminho_anterior);
                    config_salvar(g_config_path, &g_config);
                    db_fechar();
                    db_init(g_config.caminho_bd, 0);
                    db_criar_tabelas();
                    mostrar_erro(hwnd, "Nao foi possivel abrir o novo banco. Configuracao anterior restaurada.");
                }
                else
                {
                    recarregar_tipos_oleo(hwnd);
                    carregar_lista_principal(hwnd);
                    limpar_formulario(hwnd);
                    MessageBox(hwnd,
                               "Caminho do banco atualizado e reconectado com sucesso.",
                               "Configuracoes",
                               MB_OK | MB_ICONINFORMATION);
                }
            }
            break;
        }

        case IDM_CONFIG_REDE:
        {
            char path[MAX_PATH];
            OPENFILENAME ofn;
            snprintf(path, sizeof(path), "%s", g_config.caminho_rede);
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner   = hwnd;
            ofn.lpstrFile   = path;
            ofn.nMaxFile    = sizeof(path);
            ofn.lpstrFilter = "SQLite Database (*.db)\0*.db\0Todos os arquivos (*.*)\0*.*\0";
            ofn.lpstrDefExt = "db";
            ofn.lpstrTitle  = "Selecionar banco de dados de rede (sincronizacao)";
            ofn.Flags       = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
            if (GetSaveFileName(&ofn))
            {
                snprintf(g_config.caminho_rede, sizeof(g_config.caminho_rede), "%s", path);
                config_salvar(g_config_path, &g_config);
                MessageBox(hwnd,
                    "Caminho de rede configurado.\n"
                    "O banco local sera sincronizado apos cada operacao.",
                    "Rede Configurada", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }

        case IDM_SINCRONIZAR:
        {
            char hora[8];
            char msg[64];

            if (g_config.caminho_rede[0] == '\0')
            {
                MessageBox(hwnd,
                    "Nenhum caminho de rede configurado.\n"
                    "Use Configuracoes > Banco de Rede para configurar.",
                    "Sincronizacao", MB_OK | MB_ICONINFORMATION);
                break;
            }
            hora_atual(hora, sizeof(hora));
            if (db_sincronizar_para_rede(g_config.caminho_rede) == 0)
            {
                snprintf(msg, sizeof(msg), "Sincronizado com a rede — %s", hora);
                atualizar_status(hwnd, msg);
                MessageBox(hwnd, "Sincronizacao com a rede concluida.", "Sincronizacao", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                atualizar_status(hwnd, "Aviso: falha ao sincronizar com a rede");
                MessageBox(hwnd, "Falha ao sincronizar com a rede.\nVerifique a conexao.", "Sincronizacao", MB_OK | MB_ICONWARNING);
            }
            break;
        }

        case IDM_CONFIG_OLEOS:
            SetFocus(GetDlgItem(hwnd, IDC_EDIT_NOVO_OLEO));
            MessageBox(hwnd,
                       "Use os controles de gerencia de tipos na coluna direita (adicionar/remover).",
                       "Tipos de Oleo",
                       MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_RELATORIO_GERAL:
            abrir_janela_relatorio_geral(hwnd);
            break;

        case IDM_RELATORIO_VEICULOS:
            acao_ver_historico_por_placa(hwnd);
            break;

        case IDM_ABOUT:
            MessageBox(hwnd,
                       "TrocaOleo\nSistema de controle de trocas com historico por veiculo.",
                       "Sobre",
                       MB_OK | MB_ICONINFORMATION);
            break;

        case IDM_SAIR:
            DestroyWindow(hwnd);
            break;

        default:
            break;
        }
        return 0;

    case WM_SIZE:
    {
        HWND hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
        if (hStatus != NULL)
            SendMessage(hStatus, WM_SIZE, wParam, lParam);
        return 0;
    }

    case WM_DESTROY:
        db_fechar();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Opens a file picker. Returns 1 and fills path on success, 0 if cancelled. */
static int picker_arquivo_db(char *path, int path_size, int deve_existir)
{
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile   = path;
    ofn.nMaxFile    = path_size;
    ofn.lpstrFilter = "SQLite Database (*.db)\0*.db\0Todos os arquivos (*.*)\0*.*\0";
    ofn.lpstrDefExt = "db";

    if (deve_existir)
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        return GetOpenFileName(&ofn) ? 1 : 0;
    }
    else
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        return GetSaveFileName(&ofn) ? 1 : 0;
    }
}

/* Shows the first-run dialog. Returns 1 if path was chosen and config saved. */
static int dialogo_primeira_execucao(void)
{
    int escolha;
    char path[MAX_PATH];

    escolha = MessageBox(NULL,
        "Configuracao inicial detectada.\n\n"
        "Deseja:\n"
        "  [Sim]  Conectar a um banco de dados existente\n"
        "  [Nao]  Criar um novo banco de dados\n"
        "  [Cancelar]  Sair",
        "Primeira Execucao",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (escolha == IDCANCEL)
        return 0;

    snprintf(path, sizeof(path), "%s", g_config.caminho_bd);

    if (escolha == IDYES)
    {
        if (!picker_arquivo_db(path, sizeof(path), 1))
            return 0;
        snprintf(g_config.caminho_bd, sizeof(g_config.caminho_bd), "%s", path);
        config_salvar(g_config_path, &g_config);

        if (db_init(g_config.caminho_bd, 0) != 0)
        {
            MessageBox(NULL,
                "Nao foi possivel abrir o banco selecionado.",
                "Erro", MB_OK | MB_ICONERROR);
            return 0;
        }
    }
    else /* IDNO — create new */
    {
        if (!picker_arquivo_db(path, sizeof(path), 0))
            return 0;
        snprintf(g_config.caminho_bd, sizeof(g_config.caminho_bd), "%s", path);
        config_salvar(g_config_path, &g_config);
        criar_diretorios_para_arquivo(g_config.caminho_bd);

        if (db_init(g_config.caminho_bd, 1) != 0)
        {
            MessageBox(NULL,
                "Nao foi possivel criar o banco de dados.",
                "Erro", MB_OK | MB_ICONERROR);
            return 0;
        }
    }

    if (db_criar_tabelas() != 0)
    {
        MessageBox(NULL, "Erro ao preparar o banco de dados.", "Erro", MB_OK | MB_ICONERROR);
        db_fechar();
        return 0;
    }

    return 1;
}

/* Handles DB-not-found at startup. Returns 1 if successfully connected. */
static int dialogo_banco_nao_encontrado(void)
{
    char msg[MAX_PATH + 256];
    int escolha;
    char path[MAX_PATH];

    for (;;)
    {
        snprintf(msg, sizeof(msg),
            "Banco de dados nao encontrado:\n%s\n\n"
            "O arquivo pode estar em um drive de rede ainda nao conectado.\n\n"
            "  [Tentar Novamente]   Sim\n"
            "  [Configurar Caminho]   Nao\n"
            "  [Criar Novo Banco]   Cancelar",
            g_config.caminho_bd);

        escolha = MessageBox(NULL, msg, "Banco Nao Encontrado",
                             MB_YESNOCANCEL | MB_ICONWARNING);

        if (escolha == IDYES)
        {
            if (db_init(g_config.caminho_bd, 0) == 0)
            {
                if (db_criar_tabelas() != 0)
                {
                    MessageBox(NULL, "Erro ao preparar o banco.", "Erro", MB_OK | MB_ICONERROR);
                    db_fechar();
                    return 0;
                }
                return 1;
            }
            /* still not found — loop again */
            continue;
        }

        if (escolha == IDNO)
        {
            snprintf(path, sizeof(path), "%s", g_config.caminho_bd);
            if (!picker_arquivo_db(path, sizeof(path), 1))
                continue;
            snprintf(g_config.caminho_bd, sizeof(g_config.caminho_bd), "%s", path);
            config_salvar(g_config_path, &g_config);

            if (db_init(g_config.caminho_bd, 0) != 0)
            {
                MessageBox(NULL,
                    "Nao foi possivel abrir o banco selecionado.",
                    "Erro", MB_OK | MB_ICONERROR);
                continue;
            }
            if (db_criar_tabelas() != 0)
            {
                MessageBox(NULL, "Erro ao preparar o banco.", "Erro", MB_OK | MB_ICONERROR);
                db_fechar();
                return 0;
            }
            return 1;
        }

        /* IDCANCEL — create new DB */
        snprintf(path, sizeof(path), "%s", g_config.caminho_bd);
        if (!picker_arquivo_db(path, sizeof(path), 0))
            continue;
        snprintf(g_config.caminho_bd, sizeof(g_config.caminho_bd), "%s", path);
        config_salvar(g_config_path, &g_config);
        criar_diretorios_para_arquivo(g_config.caminho_bd);

        if (db_init(g_config.caminho_bd, 1) != 0)
        {
            MessageBox(NULL, "Nao foi possivel criar o banco.", "Erro", MB_OK | MB_ICONERROR);
            continue;
        }
        if (db_criar_tabelas() != 0)
        {
            MessageBox(NULL, "Erro ao preparar o banco.", "Erro", MB_OK | MB_ICONERROR);
            db_fechar();
            return 0;
        }
        return 1;
    }
}

/* Returns 1 if path starts with \\ (UNC) or refers to a non-local drive letter.
   This heuristic detects the old config where caminho_bd was the network DB. */
static int parece_caminho_de_rede(const char *path)
{
    if (path == NULL || path[0] == '\0')
        return 0;
    if (path[0] == '\\' && path[1] == '\\')
        return 1; /* UNC path */
    /* Any absolute path that is NOT the exe folder is treated as "configured" */
    return 0;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    HWND hwnd;
    int cfg_result;
    char local_path[MAX_PATH];

    (void)hPrevInstance;
    (void)lpCmdLine;

    montar_caminho_config(g_config_path, sizeof(g_config_path));
    montar_caminho_local_db(local_path, sizeof(local_path));

    cfg_result = config_carregar(g_config_path, &g_config);

    if (cfg_result == -1)
    {
        MessageBox(NULL, "Erro ao ler config.ini.", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* ---------------------------------------------------------------
       Migration: old config had caminho_bd = network path and no
       caminho_rede field. Detect and migrate automatically.
       --------------------------------------------------------------- */
    if (cfg_result == 0 &&
        g_config.caminho_rede[0] == '\0' &&
        strcmp(g_config.caminho_bd, local_path) != 0 &&
        strcmp(g_config.caminho_bd, "dados.db") != 0)
    {
        /* Old config: caminho_bd was the network/non-local DB. Migrate. */
        snprintf(g_config.caminho_rede, sizeof(g_config.caminho_rede), "%s", g_config.caminho_bd);
        snprintf(g_config.caminho_bd,   sizeof(g_config.caminho_bd),   "%s", local_path);

        /* Open (or create) local DB, then bootstrap from network */
        criar_diretorios_para_arquivo(g_config.caminho_bd);
        if (db_init(g_config.caminho_bd, 1) == 0 && db_criar_tabelas() == 0)
        {
            if (db_bootstrap_da_rede(g_config.caminho_rede) == 0)
            {
                config_salvar(g_config_path, &g_config);
                MessageBox(NULL,
                    "Configuracao atualizada:\n\n"
                    "O banco de dados agora reside localmente e sera\n"
                    "sincronizado automaticamente com a rede apos cada operacao.\n\n"
                    "Banco de rede: copiado para o banco local com sucesso.",
                    "Migracao Concluida", MB_OK | MB_ICONINFORMATION);
            }
            else
            {
                /* Bootstrap failed — rede unavailable. Keep local empty but save config. */
                config_salvar(g_config_path, &g_config);
                MessageBox(NULL,
                    "Banco de rede nao acessivel no momento.\n\n"
                    "O programa iniciara com banco local vazio.\n"
                    "Use Configuracoes > Sincronizar com Rede Agora assim que\n"
                    "a rede estiver disponivel para copiar os dados.",
                    "Rede Indisponivel", MB_OK | MB_ICONWARNING);
            }
        }
        else
        {
            MessageBox(NULL, "Erro ao criar banco local durante migracao.", "Erro", MB_OK | MB_ICONERROR);
            db_fechar();
            return 1;
        }
    }
    else if (cfg_result == 1)
    {
        /* First run — config.ini was not found */
        if (!dialogo_primeira_execucao())
            return 1;

        /* If user chose a network path as local, ask about separate network sync */
        if (g_config.caminho_rede[0] == '\0' && parece_caminho_de_rede(g_config.caminho_bd))
        {
            /* They accidentally set a UNC path as local — treat as network, bootstrap local */
            snprintf(g_config.caminho_rede, sizeof(g_config.caminho_rede), "%s", g_config.caminho_bd);
            snprintf(g_config.caminho_bd,   sizeof(g_config.caminho_bd),   "%s", local_path);
            db_fechar();
            criar_diretorios_para_arquivo(g_config.caminho_bd);
            if (db_init(g_config.caminho_bd, 1) == 0 && db_criar_tabelas() == 0)
                db_bootstrap_da_rede(g_config.caminho_rede);
            config_salvar(g_config_path, &g_config);
        }
    }
    else
    {
        /* Normal startup — try to open local DB */
        if (db_init(g_config.caminho_bd, 0) != 0)
        {
            if (!dialogo_banco_nao_encontrado())
                return 1;
        }
        else if (db_criar_tabelas() != 0)
        {
            MessageBox(NULL, "Erro ao preparar o banco.", "Erro", MB_OK | MB_ICONERROR);
            db_fechar();
            return 1;
        }

        /* Pull retorno_avisado changes made by the WhatsApp script on the network DB */
        if (g_config.caminho_rede[0] != '\0')
            g_startup_pull_rc = db_puxar_retorno_avisado(g_config.caminho_rede);
    }

    hwnd = criar_janela_principal(hInstance, nCmdShow);
    if (hwnd == NULL)
    {
        db_fechar();
        return 1;
    }

    if (g_config.caminho_rede[0] != '\0')
    {
        if (g_startup_pull_rc == 0)
            atualizar_status(hwnd, "Rede: dados atualizados na inicializacao");
        else
            atualizar_status(hwnd, "Aviso: rede indisponivel na inicializacao");
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
