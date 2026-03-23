#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "config.h"
#include "database.h"
#include "gui.h"
#include "validation.h"

static Config g_config;
static int g_edit_id = -1;

static int criar_diretorios_para_arquivo(const char* caminho_arquivo) {
    char pasta[MAX_PATH];
    size_t i;

    if (caminho_arquivo == NULL || caminho_arquivo[0] == '\0') {
        return -1;
    }

    snprintf(pasta, sizeof(pasta), "%s", caminho_arquivo);

    for (i = strlen(pasta); i > 0; i--) {
        if (pasta[i] == '\\' || pasta[i] == '/') {
            pasta[i] = '\0';
            break;
        }
    }

    if (pasta[0] == '\0') {
        return 0;
    }

    for (i = 3; pasta[i] != '\0'; i++) {
        if (pasta[i] == '\\' || pasta[i] == '/') {
            char tmp = pasta[i];
            pasta[i] = '\0';
            CreateDirectory(pasta, NULL);
            pasta[i] = tmp;
        }
    }

    CreateDirectory(pasta, NULL);
    return 0;
}

static int validar_dados_formulario(HWND hwnd, TrocaOleo* t) {
    int f;

    normalizar_placa(t->placa);

    f = validar_placa(t->placa);
    if (f == PLACA_INVALIDA) {
        mostrar_erro(hwnd,
            "Placa invalida.\n\n"
            "Formatos aceitos:\n"
            "- Mercosul: ABC1D23\n"
            "- Brasil: ABC-1234");
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_PLACA));
        return 0;
    }

    if (t->tipo_oleo[0] == '\0') {
        mostrar_erro(hwnd, "Selecione um tipo de oleo.");
        return 0;
    }

    if (t->telefone_informado && !validar_telefone(t->telefone)) {
        mostrar_erro(hwnd, "Telefone invalido. Informe 10 ou 11 digitos.");
        SetFocus(GetDlgItem(hwnd, IDC_EDIT_TELEFONE));
        return 0;
    }

    if (!validar_data(t->data_troca)) {
        mostrar_erro(hwnd, "Data da troca invalida.");
        return 0;
    }

    return 1;
}

static void carregar_lista_principal(HWND hwnd) {
    char filtro[32];
    int usar_ultimas;
    int count = 0;
    TrocaOleo* lista;
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_BUSCA), filtro, (int)sizeof(filtro));

    usar_ultimas = (Button_GetCheck(GetDlgItem(hwnd, IDC_RADIO_EXIBIR_ULTIMA)) == BST_CHECKED);

    if (usar_ultimas) {
        lista = db_listar_ultimas_trocas(filtro, &count);
    } else {
        lista = db_listar_trocas(filtro, &count);
    }

    if (lista == NULL && count == 0) {
        ListView_DeleteAllItems(hList);
        return;
    }

    atualizar_listview(hList, lista, count);
    db_liberar_trocas(lista);
}

static void acao_salvar(HWND hwnd) {
    TrocaOleo t = obter_dados_formulario(hwnd);

    if (!validar_dados_formulario(hwnd, &t)) {
        return;
    }

    if (db_inserir_troca(&t) != 0) {
        mostrar_erro(hwnd, "Falha ao salvar no banco de dados.");
        return;
    }

    carregar_lista_principal(hwnd);
    limpar_formulario(hwnd);
    mostrar_sucesso(hwnd, "Troca cadastrada com sucesso.");
}

static void acao_editar(HWND hwnd) {
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
    TrocaOleo* t = obter_item_selecionado(hList);

    if (t == NULL) {
        mostrar_erro(hwnd, "Selecione um registro para editar.");
        return;
    }

    g_edit_id = t->id;
    preencher_formulario(hwnd, t);
    db_liberar_trocas(t);
}

static void acao_atualizar(HWND hwnd) {
    TrocaOleo t = obter_dados_formulario(hwnd);

    if (g_edit_id <= 0) {
        mostrar_erro(hwnd, "Nenhum registro selecionado para atualizacao.");
        return;
    }

    if (!validar_dados_formulario(hwnd, &t)) {
        return;
    }

    if (db_atualizar_troca(g_edit_id, &t) != 0) {
        mostrar_erro(hwnd, "Falha ao atualizar registro.");
        return;
    }

    g_edit_id = -1;
    carregar_lista_principal(hwnd);
    limpar_formulario(hwnd);
    mostrar_sucesso(hwnd, "Registro atualizado com sucesso.");
}

static void acao_deletar(HWND hwnd) {
    HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
    int id = obter_id_item_selecionado(hList);

    if (id < 0) {
        mostrar_erro(hwnd, "Selecione um registro para excluir.");
        return;
    }

    if (!confirmar_acao(hwnd, "Confirmar exclusao (soft delete) do registro selecionado?")) {
        return;
    }

    if (db_deletar_troca(id) != 0) {
        mostrar_erro(hwnd, "Falha ao excluir registro.");
        return;
    }

    carregar_lista_principal(hwnd);
    mostrar_sucesso(hwnd, "Registro excluido com sucesso.");
}

static void acao_ver_historico_por_placa(HWND hwnd) {
    char placa[16];

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_PLACA), placa, (int)sizeof(placa));
    normalizar_placa(placa);

    if (placa[0] == '\0') {
        HWND hList = GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS);
        TrocaOleo* t = obter_item_selecionado(hList);
        if (t != NULL) {
            snprintf(placa, sizeof(placa), "%s", t->placa);
            db_liberar_trocas(t);
        }
    }

    abrir_janela_historico(hwnd, placa);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            criar_controles(hwnd);
            recarregar_tipos_oleo(hwnd);
            carregar_lista_principal(hwnd);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
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

                case IDC_CHECK_TELEFONE: {
                    int marcado = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_TELEFONE)) == BST_CHECKED);
                    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), marcado);
                    if (!marcado) {
                        SetWindowText(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), "");
                    }
                    break;
                }

                case IDC_BUTTON_VER_HISTORICO:
                case IDC_BUTTON_VER_HISTORICO_COMPLETO:
                    acao_ver_historico_por_placa(hwnd);
                    break;

                case IDM_CONFIG_BD:
                    config_abrir_dialogo(hwnd, &g_config, "config.ini");
                    break;

                case IDM_CONFIG_OLEOS:
                    MessageBox(hwnd,
                               "Para gerenciar tipos de oleo, altere a tabela tipos_oleo no banco ou implemente dialog dedicado.",
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

        case WM_DESTROY:
            db_fechar();
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    HWND hwnd;

    (void)hPrevInstance;
    (void)lpCmdLine;

    if (config_carregar("config.ini", &g_config) != 0) {
        MessageBox(NULL, "Nao foi possivel carregar config.ini", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    criar_diretorios_para_arquivo(g_config.caminho_bd);

    if (db_init(g_config.caminho_bd) != 0) {
        MessageBox(NULL, "Erro ao abrir banco SQLite.", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (db_criar_tabelas() != 0) {
        MessageBox(NULL, "Erro ao criar tabelas no banco.", "Erro", MB_OK | MB_ICONERROR);
        db_fechar();
        return 1;
    }

    hwnd = criar_janela_principal(hInstance, nCmdShow);
    if (hwnd == NULL) {
        db_fechar();
        return 1;
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
