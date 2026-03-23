#ifndef GUI_H
#define GUI_H

#include <windows.h>
#include "database.h"

#define IDC_EDIT_PLACA 1001
#define IDC_BUTTON_VER_HISTORICO 1002
#define IDC_RADIO_OLEO_BASE 1100
#define IDC_CHECK_TELEFONE 1200
#define IDC_EDIT_TELEFONE 1201
#define IDC_CHECK_INDICACAO 1202
#define IDC_DATETIME_TROCA 1300
#define IDC_BUTTON_SALVAR 1400
#define IDC_BUTTON_ATUALIZAR 1401
#define IDC_BUTTON_LIMPAR 1402
#define IDC_BUTTON_DELETAR 1403
#define IDC_EDIT_BUSCA 1500
#define IDC_BUTTON_PESQUISAR 1501
#define IDC_RADIO_EXIBIR_TODAS 1502
#define IDC_RADIO_EXIBIR_ULTIMA 1503
#define IDC_LISTVIEW_REGISTROS 1600
#define IDC_BUTTON_VER_HISTORICO_COMPLETO 1601
#define IDC_BUTTON_EDITAR 1602
#define IDC_EDIT_NOVO_OLEO 1700
#define IDC_BUTTON_ADICIONAR_OLEO 1701
#define IDC_BUTTON_REMOVER_OLEO 1702

#define IDM_CONFIG_BD 2001
#define IDM_CONFIG_OLEOS 2002
#define IDM_RELATORIO_VEICULOS 2003
#define IDM_RELATORIO_GERAL 2004
#define IDM_ABOUT 2005
#define IDM_SAIR 2006

HWND criar_janela_principal(HINSTANCE hInstance, int nCmdShow);
void criar_controles(HWND hwnd);
void criar_radio_buttons_oleo(HWND hwnd, TipoOleo *tipos, int count);
void recarregar_tipos_oleo(HWND hwnd);

void limpar_formulario(HWND hwnd);
void preencher_formulario(HWND hwnd, const TrocaOleo *troca);
TrocaOleo obter_dados_formulario(HWND hwnd);

void atualizar_listview(HWND hwndList, TrocaOleo *trocas, int count);
TrocaOleo *obter_item_selecionado(HWND hwndList);
int obter_id_item_selecionado(HWND hwndList);
int obter_nome_tipo_oleo_selecionado(char *nome, int nome_size);

void abrir_janela_historico(HWND hwndParent, const char *placa);
void abrir_janela_relatorio_geral(HWND hwndParent);

void mostrar_erro(HWND hwnd, const char *mensagem);
void mostrar_sucesso(HWND hwnd, const char *mensagem);
int confirmar_acao(HWND hwnd, const char *mensagem);
void mostrar_info_historico(HWND hwnd, const char *placa, int total_trocas,
                            const char *primeira, const char *ultima,
                            const char *oleo_favorito, int intervalo_dias);

HMENU criar_menu_principal(void);

#endif
