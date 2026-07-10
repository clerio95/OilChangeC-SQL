#include "gui.h"

#include <commctrl.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static HWND g_radio_oleos[64];
static int g_qtd_radios = 0;
static HFONT g_hFont = NULL;

static BOOL CALLBACK aplicar_fonte_cb(HWND hwnd, LPARAM lParam)
{
    SendMessage(hwnd, WM_SETFONT, (WPARAM)lParam, TRUE);
    return TRUE;
}

static void criar_fonte_padrao(void)
{
    NONCLIENTMETRICS ncm;

    if (g_hFont != NULL)
    {
        return;
    }

    ZeroMemory(&ncm, sizeof(ncm));
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
        g_hFont = CreateFontIndirect(&ncm.lfMessageFont);
    }
    if (g_hFont == NULL)
    {
        g_hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    }
}

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

    AppendMenu(hConfig, MF_STRING, IDM_CONFIG_BD, "Banco Local (caminho)");
    AppendMenu(hConfig, MF_STRING, IDM_CONFIG_REDE, "Banco de Rede (sincronizacao)");
    AppendMenu(hConfig, MF_STRING, IDM_SINCRONIZAR, "Sincronizar com Rede Agora");
    AppendMenu(hConfig, MF_SEPARATOR, 0, NULL);
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
        750,
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

    criar_fonte_padrao();

    CreateWindow("BUTTON", "Cadastro de Troca", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_GROUP,
                 15, 10, 940, 310, hwnd, (HMENU)IDC_GROUP_CADASTRO, NULL, NULL);

    CreateWindow("STATIC", "Placa do Veiculo:", WS_CHILD | WS_VISIBLE,
                 30, 40, 140, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                 170, 38, 130, 24, hwnd, (HMENU)IDC_EDIT_PLACA, NULL, NULL);
    CreateWindow("BUTTON", "Ver Historico", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 310, 38, 120, 24, hwnd, (HMENU)IDC_BUTTON_VER_HISTORICO, NULL, NULL);
    /* Feedback ao vivo do formato da placa enquanto o usuario digita */
    CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE,
                 440, 42, 160, 20, hwnd, (HMENU)IDC_STATIC_FORMATO_PLACA, NULL, NULL);

    CreateWindow("STATIC", "Tipo de Oleo:", WS_CHILD | WS_VISIBLE,
                 30, 74, 120, 20, hwnd, NULL, NULL, NULL);

    /* Opt-in exigido pela politica do WhatsApp: o texto deixa claro que o
       cliente esta autorizando receber aviso, nao apenas deixando o telefone */
    CreateWindow("BUTTON", "Cliente aceita aviso por WhatsApp:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX,
                 30, 178, 250, 22, hwnd, (HMENU)IDC_CHECK_TELEFONE, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                 290, 176, 150, 24, hwnd, (HMENU)IDC_EDIT_TELEFONE, NULL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), FALSE);
    CreateWindow("BUTTON", "Pediu para NAO contatar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                 460, 178, 220, 22, hwnd, (HMENU)IDC_CHECK_NAO_CONTATAR, NULL, NULL);

    CreateWindow("BUTTON", "Veio por indicacao", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                 30, 208, 170, 22, hwnd, (HMENU)IDC_CHECK_INDICACAO, NULL, NULL);

    CreateWindow("BUTTON", "Km por semana:", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                 30, 238, 150, 22, hwnd, (HMENU)IDC_CHECK_KM_SEMANAL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER,
                 190, 236, 100, 24, hwnd, (HMENU)IDC_EDIT_KM_SEMANAL, NULL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), FALSE);

    CreateWindow("STATIC", "Data da Troca:", WS_CHILD | WS_VISIBLE,
                 30, 268, 120, 20, hwnd, NULL, NULL, NULL);
    CreateWindow(DATETIMEPICK_CLASS, "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | DTS_SHORTDATEFORMAT,
                 170, 266, 130, 24, hwnd, (HMENU)IDC_DATETIME_TROCA, NULL, NULL);

    CreateWindow("BUTTON", "Salvar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 620, 38, 100, 30, hwnd, (HMENU)IDC_BUTTON_SALVAR, NULL, NULL);
    CreateWindow("BUTTON", "Atualizar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 730, 38, 100, 30, hwnd, (HMENU)IDC_BUTTON_ATUALIZAR, NULL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_ATUALIZAR), FALSE);
    CreateWindow("BUTTON", "Limpar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 840, 38, 90, 30, hwnd, (HMENU)IDC_BUTTON_LIMPAR, NULL, NULL);

    CreateWindow("BUTTON", "Registros Cadastrados", WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_GROUP,
                 15, 330, 940, 340, hwnd, (HMENU)IDC_GROUP_REGISTROS, NULL, NULL);

    CreateWindow("STATIC", "Buscar:", WS_CHILD | WS_VISIBLE,
                 30, 358, 50, 20, hwnd, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                 90, 356, 150, 24, hwnd, (HMENU)IDC_EDIT_BUSCA, NULL, NULL);
    CreateWindow("BUTTON", "Pesquisar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 250, 356, 90, 24, hwnd, (HMENU)IDC_BUTTON_PESQUISAR, NULL, NULL);

    CreateWindow("BUTTON", "Todas", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_AUTORADIOBUTTON,
                 360, 358, 90, 20, hwnd, (HMENU)IDC_RADIO_EXIBIR_TODAS, NULL, NULL);
    CreateWindow("BUTTON", "Ultima por veiculo", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                 460, 358, 150, 20, hwnd, (HMENU)IDC_RADIO_EXIBIR_ULTIMA, NULL, NULL);
    SendMessage(GetDlgItem(hwnd, IDC_RADIO_EXIBIR_TODAS), BM_SETCHECK, BST_CHECKED, 0);

    hList = CreateWindow(WC_LISTVIEW, "",
                         WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP |
                             LVS_REPORT | LVS_SINGLESEL | WS_BORDER,
                         30, 390, 900, 230,
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
    col.cx = 80;
    col.pszText = "Avisado";
    ListView_InsertColumn(hList, 6, &col);

    CreateWindow("BUTTON", "Ver Historico Completo", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 760, 356, 170, 24, hwnd, (HMENU)IDC_BUTTON_VER_HISTORICO_COMPLETO, NULL, NULL);

    /* Atuam sobre a selecao da lista, por isso ficam junto dela */
    CreateWindow("BUTTON", "Editar selecionado", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 30, 632, 160, 26, hwnd, (HMENU)IDC_BUTTON_EDITAR, NULL, NULL);
    CreateWindow("BUTTON", "Deletar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 200, 632, 100, 26, hwnd, (HMENU)IDC_BUTTON_DELETAR, NULL, NULL);

    CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                       "Pronto", hwnd, IDC_STATUSBAR);

    EnumChildWindows(hwnd, aplicar_fonte_cb, (LPARAM)g_hFont);
}

void redimensionar_controles(HWND hwnd, int cx, int cy)
{
    HWND hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
    HWND hGrpReg = GetDlgItem(hwnd, IDC_GROUP_REGISTROS);
    int sb_altura = 0;
    int grp_topo = 330;
    int grp_altura;
    int grp_base;

    if (hGrpReg == NULL)
    {
        return;
    }

    if (hStatus != NULL)
    {
        RECT rc;
        SendMessage(hStatus, WM_SIZE, 0, 0);
        GetWindowRect(hStatus, &rc);
        sb_altura = rc.bottom - rc.top;
    }

    grp_altura = cy - grp_topo - sb_altura - 8;
    if (grp_altura < 160)
    {
        grp_altura = 160;
    }
    grp_base = grp_topo + grp_altura;

    MoveWindow(GetDlgItem(hwnd, IDC_GROUP_CADASTRO), 15, 10, cx - 30, 310, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_SALVAR), cx - 360, 38, 100, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_ATUALIZAR), cx - 250, 38, 100, 30, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_LIMPAR), cx - 140, 38, 90, 30, TRUE);

    MoveWindow(hGrpReg, 15, grp_topo, cx - 30, grp_altura, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_VER_HISTORICO_COMPLETO), cx - 220, 356, 170, 24, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_LISTVIEW_REGISTROS), 30, 390, cx - 60, grp_base - 390 - 44, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_EDITAR), 30, grp_base - 36, 160, 26, TRUE);
    MoveWindow(GetDlgItem(hwnd, IDC_BUTTON_DELETAR), 200, grp_base - 36, 100, 26, TRUE);
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
        DWORD estilo = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON;

        if (i == 0)
        {
            estilo |= WS_GROUP;
        }

        g_radio_oleos[i] = CreateWindow(
            "BUTTON",
            tipos[i].nome,
            estilo,
            x,
            y,
            120,
            20,
            hwnd,
            (HMENU)(INT_PTR)(IDC_RADIO_OLEO_BASE + i),
            NULL,
            NULL);

        SendMessage(g_radio_oleos[i], WM_SETFONT, (WPARAM)g_hFont, TRUE);

        /* 6 colunas ocupando toda a largura do grupo (170..920) */
        x += 126;
        if (x + 120 > 930)
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
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_NAO_CONTATAR), BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO), BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_TELEFONE), FALSE);
    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_KM_SEMANAL), BST_UNCHECKED);
    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), "");
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), FALSE);

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

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_NAO_CONTATAR), troca->nao_contatar ? BST_CHECKED : BST_UNCHECKED);

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO), troca->veio_indicacao ? BST_CHECKED : BST_UNCHECKED);

    Button_SetCheck(GetDlgItem(hwnd, IDC_CHECK_KM_SEMANAL), troca->km_semanal_informado ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), troca->km_semanal_informado ? TRUE : FALSE);
    if (troca->km_semanal_informado && troca->km_semanal > 0)
    {
        char km_buf[16];
        snprintf(km_buf, sizeof(km_buf), "%d", troca->km_semanal);
        SetWindowText(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), km_buf);
    }
    else
    {
        SetWindowText(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), "");
    }

    for (i = 0; i < g_qtd_radios; i++)
    {
        char txt[64];
        GetWindowText(g_radio_oleos[i], txt, (int)sizeof(txt));
        Button_SetCheck(g_radio_oleos[i], strcmp(txt, troca->tipo_oleo) == 0 ? BST_CHECKED : BST_UNCHECKED);
    }

    /* Restore the original oil change date in the date picker */
    {
        int y, mo, d;
        if (sscanf(troca->data_troca, "%4d-%2d-%2d", &y, &mo, &d) == 3)
        {
            SYSTEMTIME st;
            ZeroMemory(&st, sizeof(st));
            st.wYear  = (WORD)y;
            st.wMonth = (WORD)mo;
            st.wDay   = (WORD)d;
            DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_DATETIME_TROCA), GDT_VALID, &st);
        }
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

    t.nao_contatar = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_NAO_CONTATAR)) == BST_CHECKED) ? 1 : 0;

    t.veio_indicacao = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_INDICACAO)) == BST_CHECKED) ? 1 : 0;

    t.km_semanal_informado = (Button_GetCheck(GetDlgItem(hwnd, IDC_CHECK_KM_SEMANAL)) == BST_CHECKED) ? 1 : 0;
    if (t.km_semanal_informado)
    {
        char km_buf[16];
        GetWindowText(GetDlgItem(hwnd, IDC_EDIT_KM_SEMANAL), km_buf, (int)sizeof(km_buf));
        t.km_semanal = atoi(km_buf);
    }

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
        char avisado[8];

        snprintf(buf_id, sizeof(buf_id), "%d", trocas[i].id);
        if (trocas[i].telefone_informado && trocas[i].telefone[0] != '\0')
        {
            snprintf(tel_info, sizeof(tel_info), "%s", trocas[i].telefone);
        }
        else
        {
            snprintf(tel_info, sizeof(tel_info), "-");
        }
        snprintf(indic,   sizeof(indic),   "%s", trocas[i].veio_indicacao  ? "Sim" : "Nao");
        if (trocas[i].nao_contatar)
            snprintf(avisado, sizeof(avisado), "Nunca");
        else
            snprintf(avisado, sizeof(avisado), "%s", trocas[i].retorno_avisado ? "Sim" : "Nao");
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
        ListView_SetItemText(hwndList, i, 6, avisado);
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

TrocaOleo *obter_item_selecionado(HWND hwndList)
{
    int id = obter_id_item_selecionado(hwndList);
    if (id < 0)
    {
        return NULL;
    }
    return db_buscar_troca_por_id(id);
}

void atualizar_status(HWND hwnd, const char *texto)
{
    HWND hStatus = GetDlgItem(hwnd, IDC_STATUSBAR);
    if (hStatus != NULL)
        SetWindowText(hStatus, texto);
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

/* ------------------------------------------------------------------ */
/* Dialogo modal de gerencia de tipos de oleo                          */
/* ------------------------------------------------------------------ */

static void trim_espacos(char *s)
{
    size_t len;
    size_t ini = 0;

    if (s == NULL)
    {
        return;
    }

    while (s[ini] == ' ' || s[ini] == '\t')
    {
        ini++;
    }
    if (ini > 0)
    {
        memmove(s, s + ini, strlen(s + ini) + 1);
    }

    len = strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'))
    {
        s[len - 1] = '\0';
        len--;
    }
}

static void dlg_tipos_recarregar_lista(HWND hDlg)
{
    HWND hLista = GetDlgItem(hDlg, IDC_LIST_TIPOS_OLEO);
    int count = 0;
    int i;
    TipoOleo *tipos = db_listar_tipos_oleo(&count);

    SendMessage(hLista, LB_RESETCONTENT, 0, 0);

    for (i = 0; i < count; i++)
    {
        SendMessage(hLista, LB_ADDSTRING, 0, (LPARAM)tipos[i].nome);
    }

    db_liberar_tipos(tipos);
}

static void dlg_tipos_adicionar(HWND hDlg)
{
    char nome[50];

    GetWindowText(GetDlgItem(hDlg, IDC_EDIT_NOVO_OLEO), nome, (int)sizeof(nome));
    trim_espacos(nome);

    if (nome[0] == '\0')
    {
        mostrar_erro(hDlg, "Informe um nome para o novo tipo de oleo.");
        SetFocus(GetDlgItem(hDlg, IDC_EDIT_NOVO_OLEO));
        return;
    }

    if (db_adicionar_tipo_oleo(nome) != 0)
    {
        mostrar_erro(hDlg, "Nao foi possivel cadastrar o tipo de oleo (pode ja existir).");
        return;
    }

    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_NOVO_OLEO), "");
    dlg_tipos_recarregar_lista(hDlg);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NOVO_OLEO));
}

static void dlg_tipos_remover(HWND hDlg)
{
    HWND hLista = GetDlgItem(hDlg, IDC_LIST_TIPOS_OLEO);
    char nome[50];
    int sel = (int)SendMessage(hLista, LB_GETCURSEL, 0, 0);
    int total = (int)SendMessage(hLista, LB_GETCOUNT, 0, 0);

    if (sel == LB_ERR)
    {
        mostrar_erro(hDlg, "Selecione na lista um tipo de oleo para remover.");
        return;
    }

    if (total <= 1)
    {
        mostrar_erro(hDlg, "Nao e permitido remover o ultimo tipo de oleo ativo.");
        return;
    }

    if (SendMessage(hLista, LB_GETTEXTLEN, (WPARAM)sel, 0) >= (LRESULT)sizeof(nome))
    {
        mostrar_erro(hDlg, "Nome do tipo de oleo invalido.");
        return;
    }
    SendMessage(hLista, LB_GETTEXT, (WPARAM)sel, (LPARAM)nome);

    if (!confirmar_acao(hDlg, "Remover o tipo de oleo selecionado?"))
    {
        return;
    }

    if (db_remover_tipo_oleo_por_nome(nome) != 0)
    {
        mostrar_erro(hDlg, "Nao foi possivel remover o tipo de oleo selecionado.");
        return;
    }

    dlg_tipos_recarregar_lista(hDlg);
}

static LRESULT CALLBACK DlgTiposProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_ADICIONAR_OLEO:
            dlg_tipos_adicionar(hDlg);
            return 0;
        case IDC_BUTTON_REMOVER_OLEO:
            dlg_tipos_remover(hDlg);
            return 0;
        case IDC_BUTTON_FECHAR_OLEOS:
        case IDCANCEL:
            DestroyWindow(hDlg);
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hDlg);
        return 0;
    }

    return DefWindowProc(hDlg, msg, wParam, lParam);
}

void abrir_dialogo_tipos_oleo(HWND hwndParent)
{
    static int classe_registrada = 0;
    HWND hDlg;
    RECT rcParent;
    RECT rc;
    MSG msg;
    int larg;
    int alt;

    criar_fonte_padrao();

    if (!classe_registrada)
    {
        WNDCLASS wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = DlgTiposProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "TrocaOleoDlgTipos";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClass(&wc);
        classe_registrada = 1;
    }

    /* Janela centralizada sobre a principal, com area cliente de 350x292 */
    SetRect(&rc, 0, 0, 350, 292);
    AdjustWindowRect(&rc, WS_CAPTION | WS_SYSMENU, FALSE);
    larg = rc.right - rc.left;
    alt = rc.bottom - rc.top;
    GetWindowRect(hwndParent, &rcParent);

    hDlg = CreateWindowEx(
        WS_EX_DLGMODALFRAME,
        "TrocaOleoDlgTipos",
        "Tipos de Oleo",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        rcParent.left + ((rcParent.right - rcParent.left) - larg) / 2,
        rcParent.top + ((rcParent.bottom - rcParent.top) - alt) / 2,
        larg,
        alt,
        hwndParent,
        NULL,
        GetModuleHandle(NULL),
        NULL);

    if (hDlg == NULL)
    {
        return;
    }

    CreateWindow("STATIC", "Tipos cadastrados:", WS_CHILD | WS_VISIBLE,
                 15, 10, 200, 18, hDlg, NULL, NULL, NULL);
    CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY,
                 15, 32, 320, 160, hDlg, (HMENU)IDC_LIST_TIPOS_OLEO, NULL, NULL);
    CreateWindow("STATIC", "Novo tipo:", WS_CHILD | WS_VISIBLE,
                 15, 200, 100, 18, hDlg, NULL, NULL, NULL);
    CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                 15, 220, 210, 24, hDlg, (HMENU)IDC_EDIT_NOVO_OLEO, NULL, NULL);
    CreateWindow("BUTTON", "Adicionar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 235, 220, 100, 24, hDlg, (HMENU)IDC_BUTTON_ADICIONAR_OLEO, NULL, NULL);
    CreateWindow("BUTTON", "Remover selecionado", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 15, 254, 180, 26, hDlg, (HMENU)IDC_BUTTON_REMOVER_OLEO, NULL, NULL);
    CreateWindow("BUTTON", "Fechar", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                 235, 254, 100, 26, hDlg, (HMENU)IDC_BUTTON_FECHAR_OLEOS, NULL, NULL);

    EnumChildWindows(hDlg, aplicar_fonte_cb, (LPARAM)g_hFont);
    dlg_tipos_recarregar_lista(hDlg);

    EnableWindow(hwndParent, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NOVO_OLEO));

    while (IsWindow(hDlg))
    {
        if (GetMessage(&msg, NULL, 0, 0) <= 0)
        {
            PostQuitMessage((int)msg.wParam);
            break;
        }
        if (!IsDialogMessage(hDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
}
