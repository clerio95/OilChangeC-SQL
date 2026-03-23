#include "gui.h"

#include <commctrl.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND g_radio_oleos[64];
static int g_qtd_radios = 0;

static void formatar_data_br(const char *iso, char *out, size_t out_size)
{
    int y, m, d;
    if (iso == NULL || out == NULL || out_size == 0)
    {
        return;
    }

    if (sscanf(iso, "%4d-%2d-%2d", &y, &m, &d) == 3)
    {
        snprintf(out, out_size, "%02d/%02d/%04d", d, m, y);
    }
    else
    {
        snprintf(out, out_size, "%s", iso);
    }
}

HMENU criar_menu_principal(void)
{
    HMENU hMenu = CreateMenu();
    HMENU hArquivo = CreatePopupMenu();
    HMENU hRel = CreatePopupMenu();
    HMENU hConfig = CreatePopupMenu();
    HMENU hAjuda = CreatePopupMenu();

    AppendMenu(hArquivo, MF_STRING, IDM_SAIR, "Sair");

    AppendMenu(hRel, MF_STRING, IDM_RELATORIO_VEICULOS, "Por Veiculo");
    AppendMenu(hRel, MF_STRING, IDM_RELATORIO_GERAL, "Geral");

    AppendMenu(hConfig, MF_STRING, IDM_CONFIG_BD, "Caminho do Banco");
    AppendMenu(hConfig, MF_STRING, IDM_CONFIG_OLEOS, "Tipos de Oleo");

    AppendMenu(hAjuda, MF_STRING, IDM_ABOUT, "Sobre");

    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hArquivo, "Arquivo");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hRel, "Relatorios");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hConfig, "Configuracoes");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hAjuda, "Ajuda");

    return hMenu;
}

HWND criar_janela_principal(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASS wc;
    HWND hwnd;

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "TrocaOleoWndClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        "Sistema de Controle de Troca de Oleo",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        980,
        720,
        NULL,
        criar_menu_principal(),
        hInstance,
        NULL);

    if (hwnd != NULL)
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return hwnd;
}

void criar_controles(HWND hwnd)
{
    HWND hList;
    LVCOLUMN col;
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES;
    InitCommonControlsEx(&icc);

    CreateWindow("BUTTON", "Cadastro de Troca", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                 15, 10, 940, 270, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Placa do Veiculo:", WS_CHILD | WS_VISIBLE,
                 30, 40, 140, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                 170, 38, 130, 24, hwnd, (HMENU)IDC_EDIT_PLACA, NULL, NULL);
    CreateWindow("BUTTON", "Ver Historico", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 310, 38, 120, 24, hwnd, (HMENU)IDC_BUTTON_VER_HISTORICO, NULL, NULL);

    CreateWindow("STATIC", "Tipo de Oleo:", WS_CHILD | WS_VISIBLE,
                 30, 74, 120, 20, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Gerenciar tipos:", WS_CHILD | WS_VISIBLE,
                 620, 120, 120, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                 620, 142, 200, 24, hwnd, (HMENU)IDC_EDIT_NOVO_OLEO, NULL, NULL);
    CreateWindow("BUTTON", "Adicionar tipo", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 830, 142, 100, 24, hwnd, (HMENU)IDC_BUTTON_ADICIONAR_OLEO, NULL, NULL);
    CreateWindow("BUTTON", "Remover tipo selecionado", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 620, 172, 310, 24, hwnd, (HMENU)IDC_BUTTON_REMOVER_OLEO, NULL, NULL);

    CreateWindow("BUTTON", "Informar Telefone", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                 30, 164, 150, 22, hwnd, (HMENU)IDC_CHECK_TELEFONE, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                 190, 162, 150, 24, hwnd, (HMENU)IDC_EDIT_TELEFONE, NULL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), FALSE);

    CreateWindow("BUTTON", "Veio por indicacao", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                 30, 194, 170, 22, hwnd, (HMENU)IDC_CHECK_INDICACAO, NULL, NULL);

    CreateWindow("STATIC", "Data da Troca:", WS_CHILD | WS_VISIBLE,
                 30, 226, 120, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(DATETIMEPICK_CLASS, "", WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
                 170, 222, 130, 24, hwnd, (HMENU)IDC_DATETIME_TROCA, NULL, NULL);

    CreateWindow("BUTTON", "Salvar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 620, 38, 100, 30, hwnd, (HMENU)IDC_BUTTON_SALVAR, NULL, NULL);
    CreateWindow("BUTTON", "Atualizar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 730, 38, 100, 30, hwnd, (HMENU)IDC_BUTTON_ATUALIZAR, NULL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_ATUALIZAR), FALSE);
    CreateWindow("BUTTON", "Limpar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 840, 38, 90, 30, hwnd, (HMENU)IDC_BUTTON_LIMPAR, NULL, NULL);
    CreateWindow("BUTTON", "Deletar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 620, 74, 100, 30, hwnd, (HMENU)IDC_BUTTON_DELETAR, NULL, NULL);
    CreateWindow("BUTTON", "Editar selecionado", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 730, 74, 200, 30, hwnd, (HMENU)IDC_BUTTON_EDITAR, NULL, NULL);

    CreateWindow("BUTTON", "Registros Cadastrados", WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                 15, 290, 940, 360, hwnd, NULL, NULL, NULL);

    CreateWindow("STATIC", "Buscar:", WS_CHILD | WS_VISIBLE,
                 30, 320, 50, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                 90, 318, 150, 24, hwnd, (HMENU)IDC_EDIT_BUSCA, NULL, NULL);
    CreateWindow("BUTTON", "Pesquisar", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 250, 318, 90, 24, hwnd, (HMENU)IDC_BUTTON_PESQUISAR, NULL, NULL);

    CreateWindow("BUTTON", "Todas", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                 360, 320, 90, 20, hwnd, (HMENU)IDC_RADIO_EXIBIR_TODAS, NULL, NULL);
    CreateWindow("BUTTON", "Ultima por veiculo", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                 460, 320, 150, 20, hwnd, (HMENU)IDC_RADIO_EXIBIR_ULTIMA, NULL, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_RADIO_EXIBIR_TODAS), BM_SETCHECK, BST_CHECKED, 0);

    hList = CreateWindow(WC_LISTVIEW, "",
                         WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
                         30, 350, 900, 240,
                         hwnd, (HMENU)IDC_LISTVIEW_REGISTROS, NULL, NULL);

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    col.cx = 60;
    col.pszText = "ID";
    ListView_InsertColumn(hList, 0, &col);
    col.cx = 120;
    col.pszText = "Placa";
    ListView_InsertColumn(hList, 1, &col);
    col.cx = 150;
    col.pszText = "Oleo";
    ListView_InsertColumn(hList, 2, &col);
    col.cx = 140;
    col.pszText = "Telefone";
    ListView_InsertColumn(hList, 3, &col);
    col.cx = 90;
    col.pszText = "Indicacao";
    ListView_InsertColumn(hList, 4, &col);
    col.cx = 150;
    col.pszText = "Data";
    ListView_InsertColumn(hList, 5, &col);

    CreateWindow("BUTTON", "Ver Historico Completo", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                 760, 318, 170, 24, hwnd, (HMENU)IDC_BUTTON_VER_HISTORICO_COMPLETO, NULL, NULL);
}

void criar_radio_buttons_oleo(HWND hwnd, TipoOleo *tipos, int count)
{
    int i;
    int x = 170;
    int y = 74;

    for (i = 0; i < g_qtd_radios; i++)
    {
        if (g_radio_oleos[i] != NULL)
        {
            DestroyWindow(g_radio_oleos[i]);
            g_radio_oleos[i] = NULL;
        }
    }
    g_qtd_radios = 0;

    for (i = 0; i < count && i < 64; i++)
    {
        g_radio_oleos[i] = CreateWindow(
            "BUTTON",
            tipos[i].nome,
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            x,
            y,
            120,
            20,
            hwnd,
            (HMENU)(INT_PTR)(IDC_RADIO_OLEO_BASE + i),
            NULL,
            NULL);

        x += 130;
        if (x >= 560)
        {
            x = 170;
            y += 24;
        }
    }

    g_qtd_radios = (count < 64) ? count : 64;

    if (g_qtd_radios > 0 && g_radio_oleos[0] != NULL)
    {
        SendMessage(g_radio_oleos[0], BM_SETCHECK, BST_CHECKED, 0);
    }
}

void recarregar_tipos_oleo(HWND hwnd)
{
    int count = 0;
    TipoOleo *tipos = db_listar_tipos_oleo(&count);

    if (tipos != NULL && count > 0)
    {
        criar_radio_buttons_oleo(hwnd, tipos, count);
    }

    db_liberar_tipos(tipos);
}

void limpar_formulario(HWND hwnd)
{
    SYSTEMTIME st;
    int i;

    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_PLACA), "");
    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), "");
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_TELEFONE), BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO), BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), FALSE);

    for (i = 0; i < g_qtd_radios; i++)
    {
        Button_SetCheck(g_radio_oleos[i], BST_UNCHECKED);
    }
    if (g_qtd_radios > 0)
    {
        Button_SetCheck(g_radio_oleos[0], BST_CHECKED);
    }

    GetLocalTime(&st);
    DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_DATETIME_TROCA), GDT_VALID, &st);

    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_SALVAR), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_ATUALIZAR), FALSE);
}

void preencher_formulario(HWND hwnd, const TrocaOleo *troca)
{
    int i;

    if (troca == NULL)
    {
        return;
    }

    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_PLACA), troca->placa);

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_TELEFONE), troca->telefone_informado ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), troca->telefone_informado ? TRUE : FALSE);
    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), troca->telefone);

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO), troca->veio_indicacao ? BST_CHECKED : BST_UNCHECKED);

    for (i = 0; i < g_qtd_radios; i++)
    {
        char txt[64];
        GetWindowText(g_radio_oleos[i], txt, (int)sizeof(txt));
        Button_SetCheck(g_radio_oleos[i], strcmp(txt, troca->tipo_oleo) == 0 ? BST_CHECKED : BST_UNCHECKED);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_SALVAR), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_ATUALIZAR), TRUE);
}

TrocaOleo obter_dados_formulario(HWND hwnd)
{
    TrocaOleo t;
    SYSTEMTIME st;
    int i;

    ZeroMemory(&t, sizeof(t));

    GetWindowText(GetDlgItem(hwnd, IDC_EDIT_PLACA), t.placa, (int)sizeof(t.placa));

    for (i = 0; i < g_qtd_radios; i++)
    {
        if (Button_GetCheck(g_radio_oleos[i]) == BST_CHECKED)
        {
            GetWindowText(g_radio_oleos[i], t.tipo_oleo, (int)sizeof(t.tipo_oleo));
            break;
        }
    }

    t.telefone_informado = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_TELEFONE)) == BST_CHECKED) ? 1 : 0;
    if (t.telefone_informado)
    {
        GetWindowText(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), t.telefone, (int)sizeof(t.telefone));
    }

    t.veio_indicacao = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO)) == BST_CHECKED) ? 1 : 0;

    DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATETIME_TROCA), &st);
    snprintf(t.data_troca, sizeof(t.data_troca), "%04d-%02d-%02d 00:00:00",
             st.wYear, st.wMonth, st.wDay);

    t.ativo = 1;
    return t;
}

void atualizar_listview(HWND hwndList, TrocaOleo *trocas, int count)
{
    int i;
    LVITEM item;

    ListView_DeleteAllItems(hwndList);

    for (i = 0; i < count; i++)
    {
        char buf_id[32];
        char tel_info[32];
        char indic[8];
        char data_br[32];

        snprintf(buf_id, sizeof(buf_id), "%d", trocas[i].id);
        if (trocas[i].telefone_informado && trocas[i].telefone[0] != '\0')
        {
            snprintf(tel_info, sizeof(tel_info), "%s", trocas[i].telefone);
        }
        else
        {
            snprintf(tel_info, sizeof(tel_info), "-");
        }
        snprintf(indic, sizeof(indic), "%s", trocas[i].veio_indicacao ? "Sim" : "Nao");
        formatar_data_br(trocas[i].data_troca, data_br, sizeof(data_br));

        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = buf_id;
        item.lParam = trocas[i].id;

        ListView_InsertItem(hwndList, &item);
        ListView_SetItemText(hwndList, i, 1, trocas[i].placa);
        ListView_SetItemText(hwndList, i, 2, trocas[i].tipo_oleo);
        ListView_SetItemText(hwndList, i, 3, tel_info);
        ListView_SetItemText(hwndList, i, 4, indic);
        ListView_SetItemText(hwndList, i, 5, data_br);
    }
}

int obter_id_item_selecionado(HWND hwndList)
{
    int idx = ListView_GetNextItem(hwndList, -1, LVNI_SELECTED);
    LVITEM item;

    if (idx < 0)
    {
        return -1;
    }

    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = idx;

    if (!ListView_GetItem(hwndList, &item))
    {
        return -1;
    }

    return (int)item.lParam;
}

int obter_nome_tipo_oleo_selecionado(char *nome, int nome_size)
{
    int i;

    if (nome == NULL || nome_size <= 0)
    {
        return 0;
    }

    nome[0] = '\0';

    for (i = 0; i < g_qtd_radios; i++)
    {
        if (Button_GetCheck(g_radio_oleos[i]) == BST_CHECKED)
        {
            GetWindowText(g_radio_oleos[i], nome, nome_size);
            return nome[0] != '\0';
        }
    }

    return 0;
}

TrocaOleo *obter_item_selecionado(HWND hwndList)
{
    int id = obter_id_item_selecionado(hwndList);
    if (id < 0)
    {
        return NULL;
    }
    return db_buscar_troca_por_id(id);
}

void mostrar_erro(HWND hwnd, const char *mensagem)
{
    MessageBox(hwnd, mensagem, "Erro", MB_OK | MB_ICONERROR);
}

void mostrar_sucesso(HWND hwnd, const char *mensagem)
{
    MessageBox(hwnd, mensagem, "Sucesso", MB_OK | MB_ICONINFORMATION);
}

int confirmar_acao(HWND hwnd, const char *mensagem)
{
    return MessageBox(hwnd, mensagem, "Confirmacao", MB_YESNO | MB_ICONQUESTION) == IDYES;
}

void mostrar_info_historico(HWND hwnd, const char *placa, int total_trocas,
                            const char *primeira, const char *ultima,
                            const char *oleo_favorito, int intervalo_dias)
{
    char msg[1024];
    char p1[32];
    char p2[32];

    formatar_data_br(primeira, p1, sizeof(p1));
    formatar_data_br(ultima, p2, sizeof(p2));

    snprintf(msg, sizeof(msg),
             "Veiculo: %s\n"
             "Total de trocas: %d\n"
             "Primeira troca: %s\n"
             "Ultima troca: %s\n"
             "Intervalo medio: %d dias\n"
             "Oleo mais usado: %s",
             placa,
             total_trocas,
             (primeira && primeira[0]) ? p1 : "-",
             (ultima && ultima[0]) ? p2 : "-",
             intervalo_dias,
             (oleo_favorito && oleo_favorito[0]) ? oleo_favorito : "-");

    MessageBox(hwnd, msg, "Historico do Veiculo", MB_OK | MB_ICONINFORMATION);
}

void abrir_janela_historico(HWND hwndParent, const char *placa)
{
    int total;
    int intervalo;
    char *primeira;
    char *ultima;
    char *favorito;

    if (placa == NULL || placa[0] == '\0')
    {
        mostrar_erro(hwndParent, "Informe ou selecione uma placa para ver o historico.");
        return;
    }

    total = db_contar_trocas_por_placa(placa);
    primeira = db_data_primeira_troca(placa);
    ultima = db_data_ultima_troca(placa);
    favorito = db_tipo_oleo_mais_usado(placa);
    intervalo = db_intervalo_medio_dias(placa);

    mostrar_info_historico(hwndParent, placa, total,
                           primeira ? primeira : "",
                           ultima ? ultima : "",
                           favorito ? favorito : "",
                           intervalo);

    free(primeira);
    free(ultima);
    free(favorito);
}

void abrir_janela_relatorio_geral(HWND hwndParent)
{
    int total_veiculos = db_total_veiculos_cadastrados();
    int total_trocas = db_total_trocas_realizadas();
    char msg[512];

    snprintf(msg, sizeof(msg),
             "=== RELATORIO GERAL ===\n\n"
             "Total de veiculos unicos: %d\n"
             "Total de trocas ativas: %d\n"
             "Media de trocas por veiculo: %.2f",
             total_veiculos,
             total_trocas,
             (total_veiculos > 0) ? ((double)total_trocas / (double)total_veiculos) : 0.0);

    MessageBox(hwndParent, msg, "Relatorio Geral", MB_OK | MB_ICONINFORMATION);
}
