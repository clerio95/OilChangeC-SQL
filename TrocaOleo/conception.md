# PROMPT COMPLETO - Sistema de Controle de Troca de Óleo em C

## OBJETIVO
Criar um programa desktop em C para Windows que registre trocas de óleo de veículos em banco de dados SQLite, com GUI nativa Win32 API, validação de placas Mercosul/Brasil, e sistema CRUD completo.

---

## ESPECIFICAÇÕES TÉCNICAS

### Linguagem e APIs
- **Linguagem**: C (C11 ou superior)
- **GUI**: Win32 API nativa (sem dependências externas)
- **Banco de dados**: SQLite3 (arquivo .db)
- **Compilador**: MinGW-GCC ou MSVC
- **Sistema operacional**: Windows 10/11

### Arquitetura do Projeto
```
TrocaOleo/
├── src/
│   ├── main.c              # Código principal com WinMain
│   ├── database.c          # Funções de banco de dados SQLite
│   ├── database.h          # Header do módulo de BD
│   ├── gui.c               # Funções de interface Win32
│   ├── gui.h               # Header da GUI
│   ├── validation.c        # Validação de placa e campos
│   ├── validation.h        # Header de validação
│   └── config.c            # Gerenciamento de configurações
│   └── config.h            # Header de configurações
├── include/
│   └── sqlite3.h           # Header SQLite
├── lib/
│   └── sqlite3.dll         # Biblioteca SQLite (Windows)
├── Makefile                # Script de compilação
├── README.md               # Documentação
└── config.ini              # Arquivo de configuração (caminho do BD)
```

---

## ESTRUTURA DO BANCO DE DADOS

### ⚠️ IMPORTANTE: SISTEMA DE HISTÓRICO
O sistema **permite múltiplos registros da mesma placa** para manter histórico completo de trocas de óleo de cada veículo ao longo do tempo. Não há restrição de unicidade na placa.

### Tabela: trocas_oleo (histórico completo)
```sql
CREATE TABLE IF NOT EXISTS trocas_oleo (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    placa TEXT NOT NULL,                   -- Permite duplicatas (histórico)
    tipo_oleo TEXT NOT NULL,
    telefone TEXT,
    telefone_informado INTEGER DEFAULT 0,  -- 0=não, 1=sim (checkbox)
    veio_indicacao INTEGER DEFAULT 0,      -- 0=não, 1=sim (checkbox)
    data_troca TEXT NOT NULL,              -- formato: YYYY-MM-DD HH:MM:SS
    data_cadastro TEXT DEFAULT CURRENT_TIMESTAMP,
    ativo INTEGER DEFAULT 1                -- soft delete: 0=deletado, 1=ativo
);

-- Índices para performance em buscas de histórico
CREATE INDEX IF NOT EXISTS idx_placa ON trocas_oleo(placa);
CREATE INDEX IF NOT EXISTS idx_data_troca ON trocas_oleo(data_troca DESC);
CREATE INDEX IF NOT EXISTS idx_placa_data ON trocas_oleo(placa, data_troca DESC);

-- View para última troca de cada veículo (útil para resumos)
CREATE VIEW IF NOT EXISTS ultimas_trocas AS
SELECT t1.* 
FROM trocas_oleo t1
INNER JOIN (
    SELECT placa, MAX(data_troca) as max_data
    FROM trocas_oleo
    WHERE ativo = 1
    GROUP BY placa
) t2 ON t1.placa = t2.placa AND t1.data_troca = t2.max_data
WHERE t1.ativo = 1;
```

### Tabela: tipos_oleo (configurável pelo usuário)
```sql
CREATE TABLE IF NOT EXISTS tipos_oleo (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    nome TEXT NOT NULL UNIQUE,
    ativo INTEGER DEFAULT 1
);

-- Dados iniciais de exemplo
INSERT OR IGNORE INTO tipos_oleo (nome) VALUES 
    ('Mineral'),
    ('Semissintético'),
    ('Sintético'),
    ('5W30'),
    ('10W40');
```

### Tabela: configuracoes
```sql
CREATE TABLE IF NOT EXISTS configuracoes (
    chave TEXT PRIMARY KEY,
    valor TEXT NOT NULL
);

-- Configuração inicial do caminho do BD
INSERT OR IGNORE INTO configuracoes (chave, valor) VALUES 
    ('caminho_bd', 'C:\TrocaOleo\dados.db');
```

---

## INTERFACE GRÁFICA (WIN32 API)

### Janela Principal - Layout
```
┌─────────────────────────────────────────────────────────┐
│  Sistema de Controle de Troca de Óleo           [_][□][X]│
├─────────────────────────────────────────────────────────┤
│                                                           │
│  ┌─ Cadastro de Troca ────────────────────────────────┐ │
│  │                                                      │ │
│  │  Placa do Veículo: [____________]  [Ver Histórico] │ │
│  │  Formatos aceitos: ABC1D23 (Mercosul) ou ABC-1234  │ │
│  │                                                      │ │
│  │  Tipo de Óleo:                                      │ │
│  │    ○ Mineral    ○ Semissintético    ○ Sintético    │ │
│  │    ○ 5W30       ○ 10W40            [+ Configurar]  │ │
│  │                                                      │ │
│  │  ☐ Informar Telefone                               │ │
│  │     Telefone: [_______________] (opcional)          │ │
│  │                                                      │ │
│  │  ☐ Veio por indicação                              │ │
│  │                                                      │ │
│  │  Data da Troca: [DD/MM/YYYY] [Hoje]               │ │
│  │                                                      │ │
│  │  [  Salvar  ] [Atualizar] [ Limpar ] [ Deletar ]  │ │
│  └──────────────────────────────────────────────────── ┘│
│                                                           │
│  ┌─ Registros Cadastrados ──────────────────────────── ┐│
│  │ Buscar: [____________] [🔍 Pesquisar]               ││
│  │ Exibir: ○ Todas  ○ Última troca por veículo         ││
│  │                                                       ││
│  │ ┌───────────────────────────────────────────────┐  ││
│  │ │ ID | Placa    | Óleo    | Tel.  | Data       │  ││
│  │ │─────────────────────────────────────────────── │  ││
│  │ │ 5  | ABC1D23  | 5W30    | Sim   | 23/03/2026 │  ││
│  │ │ 4  | ABC1D23  | Mineral | Sim   | 23/12/2025 │  ││
│  │ │ 3  | XYZ-9876 | 10W40   | Não   | 22/03/2026 │  ││
│  │ │ 2  | ABC1D23  | 5W30    | Sim   | 15/09/2025 │  ││
│  │ └───────────────────────────────────────────────┘  ││
│  │                                                       ││
│  │ [ Editar ] [ Excluir ] [ Ver Histórico Completo ]  ││
│  └───────────────────────────────────────────────────── ┘│
│                                                           │
│  Menu: [Arquivo] [Relatórios] [Configurações] [Ajuda]   │
└───────────────────────────────────────────────────────────┘
```

### Janela de Histórico por Veículo (Dialog Box)
```
┌─────────────────────────────────────────────────────────┐
│  Histórico de Trocas - ABC1D23                   [_][X] │
├─────────────────────────────────────────────────────────┤
│                                                           │
│  Veículo: ABC1D23                                        │
│  Total de trocas: 3                                      │
│  Primeira troca: 15/09/2025                              │
│  Última troca: 23/03/2026                                │
│                                                           │
│  ┌───────────────────────────────────────────────────┐  │
│  │ # | Data       | Óleo     | Tel.  | Indicação   │  │
│  │───────────────────────────────────────────────────│  │
│  │ 5 | 23/03/2026 | 5W30     | Sim   | Não         │  │
│  │ 4 | 23/12/2025 | Mineral  | Sim   | Não         │  │
│  │ 2 | 15/09/2025 | 5W30     | Sim   | Sim         │  │
│  └───────────────────────────────────────────────────┘  │
│                                                           │
│  Intervalo médio entre trocas: 93 dias (~3 meses)       │
│  Óleo mais usado: 5W30 (2x)                              │
│                                                           │
│  [ Exportar Histórico ] [ Imprimir ] [ Fechar ]         │
└───────────────────────────────────────────────────────────┘
```

### Componentes Win32 API Necessários

#### Controles:
- **EDIT** (caixas de texto): placa, telefone, busca
- **BUTTON**: Salvar, Atualizar, Limpar, Deletar, Pesquisar, Validar
- **CHECKBOX**: telefone_informado, veio_indicacao
- **RADIOBUTTON**: tipos de óleo (dinâmicos)
- **LISTVIEW** (LVS_REPORT): tabela de registros
- **DATETIMEPICKER**: seleção de data
- **STATIC**: labels dos campos

#### IDs dos Controles (defines):
```c
#define IDC_EDIT_PLACA          1001
#define IDC_BUTTON_VER_HISTORICO 1002
#define IDC_RADIO_OLEO_BASE     1100  // Base para radio buttons dinâmicos
#define IDC_CHECK_TELEFONE      1200
#define IDC_EDIT_TELEFONE       1201
#define IDC_CHECK_INDICACAO     1202
#define IDC_DATETIME_TROCA      1300
#define IDC_BUTTON_SALVAR       1400
#define IDC_BUTTON_ATUALIZAR    1401
#define IDC_BUTTON_LIMPAR       1402
#define IDC_BUTTON_DELETAR      1403
#define IDC_EDIT_BUSCA          1500
#define IDC_BUTTON_PESQUISAR    1501
#define IDC_RADIO_EXIBIR_TODAS  1502
#define IDC_RADIO_EXIBIR_ULTIMA 1503
#define IDC_LISTVIEW_REGISTROS  1600
#define IDC_BUTTON_VER_HISTORICO_COMPLETO 1601
#define IDM_CONFIG_BD           2001  // Menu: Configurações > Caminho BD
#define IDM_CONFIG_OLEOS        2002  // Menu: Configurações > Tipos de Óleo
#define IDM_RELATORIO_VEICULOS  2003  // Menu: Relatórios > Por Veículo
#define IDM_RELATORIO_GERAL     2004  // Menu: Relatórios > Geral
#define IDM_ABOUT               2005  // Menu: Ajuda > Sobre
```

---

## VALIDAÇÃO DE PLACA

### Regras de Validação

#### Formato Mercosul (atual):
- **Padrão**: `ABC1D23` (3 letras + 1 número + 1 letra + 2 números)
- **Regex**: `^[A-Z]{3}[0-9][A-Z][0-9]{2}$`
- **Exemplo válido**: `ABC1D23`, `XYZ9K87`

#### Formato Brasil (antigo):
- **Padrão**: `ABC-1234` (3 letras + hífen + 4 números)
- **Regex**: `^[A-Z]{3}-[0-9]{4}$`
- **Exemplo válido**: `ABC-1234`, `XYZ-9876`

### Função de Validação (pseudo-código):
```c
int validar_placa(const char* placa) {
    // Converter para maiúsculas
    // Remover espaços extras
    
    // Verificar formato Mercosul: ABC1D23
    if (regex_match(placa, "^[A-Z]{3}[0-9][A-Z][0-9]{2}$"))
        return PLACA_MERCOSUL;
    
    // Verificar formato Brasil: ABC-1234
    if (regex_match(placa, "^[A-Z]{3}-[0-9]{4}$"))
        return PLACA_BRASIL;
    
    return PLACA_INVALIDA;
}
```

---

## SISTEMA DE HISTÓRICO POR VEÍCULO

### Conceito Principal
**A mesma placa pode ter múltiplos registros** para rastrear todas as trocas de óleo ao longo do tempo. Não há constraint UNIQUE na coluna placa.

### Exemplo de Histórico:
```
Placa: ABC1D23

ID  | Data Troca  | Tipo Óleo      | Telefone | Indicação
----|-------------|----------------|----------|----------
8   | 23/03/2026  | 5W30           | Sim      | Não
6   | 15/12/2025  | Mineral        | Sim      | Não
3   | 10/09/2025  | Semissintético | Sim      | Sim
1   | 05/06/2025  | 5W30           | Não      | Sim

Estatísticas:
- Total de trocas: 4
- Primeira troca: 05/06/2025
- Última troca: 23/03/2026
- Intervalo médio: ~90 dias
- Óleo preferido: 5W30 (2x)
- Cliente fiel: Sim (4 visitas em 9 meses)
```

### Queries SQL Importantes:

#### 1. Buscar histórico completo de uma placa:
```sql
SELECT * FROM trocas_oleo 
WHERE placa = 'ABC1D23' AND ativo = 1 
ORDER BY data_troca DESC;
```

#### 2. Contar trocas de um veículo:
```sql
SELECT COUNT(*) FROM trocas_oleo 
WHERE placa = 'ABC1D23' AND ativo = 1;
```

#### 3. Tipo de óleo mais usado por um veículo:
```sql
SELECT tipo_oleo, COUNT(*) as vezes 
FROM trocas_oleo 
WHERE placa = 'ABC1D23' AND ativo = 1 
GROUP BY tipo_oleo 
ORDER BY vezes DESC 
LIMIT 1;
```

#### 4. Intervalo médio entre trocas (em dias):
```sql
WITH trocas_ordenadas AS (
    SELECT data_troca,
           LAG(data_troca) OVER (ORDER BY data_troca) as troca_anterior
    FROM trocas_oleo
    WHERE placa = 'ABC1D23' AND ativo = 1
)
SELECT AVG(julianday(data_troca) - julianday(troca_anterior)) as intervalo_medio
FROM trocas_ordenadas
WHERE troca_anterior IS NOT NULL;
```

#### 5. Listar todas as placas únicas cadastradas:
```sql
SELECT DISTINCT placa 
FROM trocas_oleo 
WHERE ativo = 1 
ORDER BY placa;
```

#### 6. Total de veículos únicos:
```sql
SELECT COUNT(DISTINCT placa) FROM trocas_oleo WHERE ativo = 1;
```

### Relatórios Possíveis:

#### Relatório Geral:
```
=== RELATÓRIO GERAL ===
Período: 01/01/2025 a 23/03/2026

Estatísticas Gerais:
- Total de veículos atendidos: 45
- Total de trocas realizadas: 127
- Média de trocas por veículo: 2.8

Tipos de Óleo Mais Usados:
1. 5W30: 48 trocas (37.8%)
2. Mineral: 35 trocas (27.6%)
3. Semissintético: 28 trocas (22%)
4. Sintético: 16 trocas (12.6%)

Veículos com Mais Trocas:
1. ABC1D23: 8 trocas
2. XYZ-9876: 6 trocas
3. DEF2G34: 5 trocas

Indicações: 42 clientes vieram por indicação (33%)
```

#### Relatório por Veículo:
```
=== HISTÓRICO DETALHADO ===
Veículo: ABC1D23

Resumo:
- Total de trocas: 8
- Primeira visita: 05/06/2025
- Última visita: 23/03/2026
- Período: 9 meses e 18 dias
- Intervalo médio: 42 dias (~1.4 meses)
- Óleo preferido: 5W30 (5x)

Histórico Completo:
[Tabela com todas as 8 trocas ordenadas por data DESC]

Observações:
✓ Cliente regular (8 visitas em 9 meses)
✓ Prefere óleo 5W30
✓ Sempre informa telefone
```

---

## FUNCIONALIDADES CRUD

### 1. CREATE (Inserir)
```c
//Função principal, página principal do programa
// Ao clicar em "Salvar":
1. Validar placa (Mercosul ou Brasil)
2. Verificar se tipo de óleo foi selecionado
3. Se checkbox "Informar Telefone" marcado, validar telefone
4. Capturar data selecionada (padrão: hoje)
5. Executar INSERT no SQLite:
   INSERT INTO trocas_oleo (placa, tipo_oleo, telefone, 
                             telefone_informado, veio_indicacao, data_troca)
   VALUES (?, ?, ?, ?, ?, ?);
6. Atualizar ListView com novo registro
7. Limpar formulário
8. Mostrar mensagem de sucesso
```

### 2. READ (Listar/Pesquisar)
```c
// Ao clicar "Pesquisar":
// MODO 1: Exibir TODAS as trocas (histórico completo)
if (radio_todas_selecionado) {
    if (campo_busca_vazio) {
        SELECT * FROM trocas_oleo 
        WHERE ativo=1 
        ORDER BY data_troca DESC;
    } else {
        SELECT * FROM trocas_oleo 
        WHERE ativo=1 AND placa LIKE '%?%' 
        ORDER BY data_troca DESC;
    }
}

// MODO 2: Exibir ÚLTIMA troca de cada veículo (resumo)
if (radio_ultima_selecionado) {
    if (campo_busca_vazio) {
        SELECT * FROM ultimas_trocas 
        ORDER BY data_troca DESC;
    } else {
        SELECT * FROM ultimas_trocas 
        WHERE placa LIKE '%?%' 
        ORDER BY data_troca DESC;
    }
}

// Popular ListView com resultados
// Exibir: "Mostrando X registros (Y veículos únicos)"
```

### 2.1. VER HISTÓRICO COMPLETO DE UM VEÍCULO
```c
// Ao clicar no botão "Ver Histórico" ao lado da placa OU
// Ao clicar no botão "Ver Histórico Completo" com item selecionado:

1. Capturar placa do campo OU do item selecionado no ListView
2. Abrir janela de diálogo (Dialog Box) de histórico
3. Executar query:
   SELECT * FROM trocas_oleo 
   WHERE placa=? AND ativo=1 
   ORDER BY data_troca DESC;
4. Popular ListView da janela de histórico
5. Calcular estatísticas:
   - Total de trocas
   - Data primeira/última troca
   - Intervalo médio entre trocas
   - Tipo de óleo mais usado
6. Exibir botões: [Exportar] [Imprimir] [Fechar]
```

### 3. UPDATE (Atualizar)
```c
// Ao selecionar registro no ListView e clicar "Editar Selecionado":
1. Carregar dados do registro nos campos do formulário
2. Desabilitar botão "Salvar"
3. Habilitar botão "Atualizar"
4. Armazenar ID do registro em variável global

// Ao clicar em "Atualizar":
1. Validar todos os campos novamente
2. Executar UPDATE no SQLite:
   UPDATE trocas_oleo 
   SET placa=?, tipo_oleo=?, telefone=?, telefone_informado=?, 
       veio_indicacao=?, data_troca=?
   WHERE id=?;
3. Atualizar ListView
4. Limpar formulário e resetar botões
```

### 4. DELETE (Soft Delete)
```c
// Ao clicar em "Excluir Selecionado":
1. Confirmar exclusão com MessageBox
2. Executar soft delete no SQLite:
   UPDATE trocas_oleo SET ativo=0 WHERE id=?;
3. Remover item do ListView
4. Mostrar mensagem de confirmação
```

---

## SISTEMA DE CONFIGURAÇÃO

### Arquivo config.ini (formato INI padrão):
```ini
[Database]
caminho=C:\TrocaOleo\dados.db

[Interface]
tema=claro
fonte_tamanho=10

[Backup]
auto_backup=1
pasta_backup=C:\TrocaOleo\Backups
```

### Janela de Configuração (Dialog Box):
```
┌─────────────────────────────────────┐
│  Configurações               [_][X] │
├─────────────────────────────────────┤
│                                     │
│  Caminho do Banco de Dados:        │
│  [C:\TrocaOleo\dados.db____] [...]  │
│                                     │
│  ☐ Criar backup automático diário  │
│                                     │
│  [ Testar Conexão ]                │
│                                     │
│  [   OK   ]  [ Cancelar ]          │
└─────────────────────────────────────┘
```

### Gerenciamento de Tipos de Óleo (Dialog Box):
```
┌─────────────────────────────────────┐
│  Configurar Tipos de Óleo    [_][X] │
├─────────────────────────────────────┤
│                                     │
│  Tipos Cadastrados:                │
│  ┌───────────────────────────────┐ │
│  │ ☑ Mineral                     │ │
│  │ ☑ Semissintético              │ │
│  │ ☑ Sintético                   │ │
│  │ ☑ 5W30                        │ │
│  │ ☑ 10W40                       │ │
│  └───────────────────────────────┘ │
│                                     │
│  Novo tipo: [______________]       │
│  [ Adicionar ] [ Remover ]         │
│                                     │
│  [  Salvar  ]  [ Cancelar ]        │
└─────────────────────────────────────┘
```

---

## FLUXO DE EXECUÇÃO DO PROGRAMA

### Inicialização:
```
1. WinMain() inicia
2. Carregar config.ini
3. Verificar se caminho do BD existe
   - Se não: criar pasta e arquivo .db
4. Conectar ao SQLite (sqlite3_open)
5. Criar tabelas se não existirem
6. Carregar tipos de óleo do BD
7. Criar janela principal (CreateWindow)
8. Criar controles (EDIT, BUTTON, etc)
9. Criar radio buttons dinâmicos para tipos de óleo
10. Carregar registros no ListView
11. Entrar no loop de mensagens (GetMessage)
```

### Loop Principal (WndProc):
```c
switch (msg) {
    case WM_CREATE:
        // Criar todos os controles
        break;
    
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case IDC_BUTTON_SALVAR:
                // Validar e inserir no BD
                break;
            case IDC_BUTTON_ATUALIZAR:
                // Validar e atualizar registro
                break;
            case IDC_CHECK_TELEFONE:
                // Habilitar/desabilitar campo telefone
                break;
            // ... outros comandos
        }
        break;
    
    case WM_NOTIFY:
        // Eventos do ListView (clique duplo, seleção)
        break;
    
    case WM_DESTROY:
        sqlite3_close(db);
        PostQuitMessage(0);
        break;
}
```

---

## FUNÇÕES PRINCIPAIS A IMPLEMENTAR

### database.c
```c
// Inicialização
int db_init(const char* db_path);
int db_criar_tabelas();

// CRUD trocas_oleo
int db_inserir_troca(TrocaOleo* troca);
TrocaOleo* db_listar_trocas(const char* filtro_placa, int* count);
TrocaOleo* db_listar_ultimas_trocas(int* count);  // Nova: usa VIEW ultimas_trocas
int db_atualizar_troca(int id, TrocaOleo* troca);
int db_deletar_troca(int id);

// Histórico por veículo (NOVAS FUNÇÕES)
TrocaOleo* db_historico_por_placa(const char* placa, int* count);
int db_contar_trocas_por_placa(const char* placa);
char* db_tipo_oleo_mais_usado(const char* placa);
int db_intervalo_medio_dias(const char* placa);
char* db_data_primeira_troca(const char* placa);
char* db_data_ultima_troca(const char* placa);

// Relatórios e estatísticas (NOVAS)
int db_total_veiculos_cadastrados();
int db_total_trocas_realizadas();
char** db_listar_placas_unicas(int* count);  // Todas as placas distintas

// Tipos de óleo
TipoOleo* db_listar_tipos_oleo(int* count);
int db_adicionar_tipo_oleo(const char* nome);
int db_remover_tipo_oleo(int id);

// Cleanup
void db_fechar();
```

### gui.c
```c
// Criação de interface
HWND criar_janela_principal(HINSTANCE hInstance);
void criar_controles(HWND hwnd);
void criar_radio_buttons_oleo(HWND hwnd, TipoOleo* tipos, int count);

// Manipulação de dados
void limpar_formulario(HWND hwnd);
void preencher_formulario(HWND hwnd, TrocaOleo* troca);
TrocaOleo obter_dados_formulario(HWND hwnd);

// ListView
void atualizar_listview(HWND hwndList, TrocaOleo* trocas, int count);
void atualizar_listview_ultimas(HWND hwndList);  // Nova: exibe só última troca
TrocaOleo* obter_item_selecionado(HWND hwndList);

// Janelas de diálogo (NOVAS)
void abrir_janela_historico(HWND hwndParent, const char* placa);
void abrir_janela_relatorio_geral(HWND hwndParent);

// Mensagens
void mostrar_erro(const char* mensagem);
void mostrar_sucesso(const char* mensagem);
int confirmar_acao(const char* mensagem);
void mostrar_info_historico(const char* placa, int total_trocas, 
                            const char* primeira, const char* ultima,
                            const char* oleo_favorito, int intervalo_dias);
```

### validation.c
```c
// Validações
int validar_placa(const char* placa);
int validar_telefone(const char* telefone);
int validar_data(const char* data);
void normalizar_placa(char* placa);  // Converter para maiúsculas
```

### config.c
```c
// Configurações
int config_carregar(const char* config_path, Config* config);
int config_salvar(const char* config_path, Config* config);
void config_abrir_dialogo(HWND hwndParent);
```

---

## ESTRUTURAS DE DADOS

```c
// Estrutura principal de troca de óleo
typedef struct {
    int id;
    char placa[10];              // "ABC1D23" ou "ABC-1234"
    char tipo_oleo[50];
    char telefone[20];
    int telefone_informado;      // 0 ou 1
    int veio_indicacao;          // 0 ou 1
    char data_troca[20];         // "YYYY-MM-DD HH:MM:SS"
    char data_cadastro[20];
    int ativo;                   // 0=deletado, 1=ativo
} TrocaOleo;

// Estrutura de tipo de óleo
typedef struct {
    int id;
    char nome[50];
    int ativo;
} TipoOleo;

// Estrutura de configuração
typedef struct {
    char caminho_bd[MAX_PATH];
    char tema[20];
    int fonte_tamanho;
    int auto_backup;
    char pasta_backup[MAX_PATH];
} Config;

// Formatos de placa
typedef enum {
    PLACA_INVALIDA = 0,
    PLACA_MERCOSUL = 1,
    PLACA_BRASIL = 2
} FormatoPlaca;
```

---

## COMPILAÇÃO

### Makefile (MinGW):
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./include
LDFLAGS = -L./lib -lsqlite3 -lcomctl32 -lgdi32 -lcomdlg32
TARGET = TrocaOleo.exe

SRCS = src/main.c src/database.c src/gui.c src/validation.c src/config.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Compilação concluída: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	del /Q src\*.o $(TARGET)

run: $(TARGET)
	.\$(TARGET)

.PHONY: all clean run
```

### Comandos de Compilação:
```bash
# Compilar
mingw32-make

# Limpar objetos
mingw32-make clean

# Compilar e executar
mingw32-make run
```

---

## ATALHO DO WINDOWS

### Criar Atalho com Tecla de Atalho:

1. **Criar atalho do TrocaOleo.exe**:
   - Botão direito no executável → "Criar atalho"

2. **Configurar tecla de atalho**:
   - Botão direito no atalho → "Propriedades"
   - Campo "Tecla de atalho": `Ctrl + Alt + T` (ou qualquer combinação)
   - Aplicar e OK

3. **Mover atalho para local acessível**:
   - `C:\Users\<Usuario>\AppData\Roaming\Microsoft\Windows\Start Menu\Programs`
   - Ou área de trabalho

### Executar via linha de comando:
```batch
start "" "C:\TrocaOleo\TrocaOleo.exe"
```

---

## RECURSOS ADICIONAIS

### Melhorias Sugeridas (futuras):
1. **Exportar relatórios**: CSV, PDF, Excel (histórico por veículo ou geral)
2. **Gráficos**: 
   - Tipos de óleo mais usados (pizza)
   - Trocas por mês (linha)
   - Evolução de clientes ao longo do tempo
   - Frequência de retorno dos clientes
3. **Notificações/Alertas**: 
   - Avisar quando veículo está atrasado para próxima troca (baseado no intervalo médio)
   - Lembrete de cliente que não retorna há muito tempo
4. **Análise de histórico avançada**: 
   - Identificar clientes fiéis (5+ trocas)
   - Clientes em risco de abandono (não voltam há 6+ meses)
   - Taxa de retorno por período
5. **Dashboard inicial**: 
   - Total de trocas hoje/semana/mês
   - Veículos atendidos hoje
   - Top 5 óleos mais usados
   - Gráfico de trocas nos últimos 12 meses
6. **Backup automático**: agendar backup diário do .db
7. **Multi-usuário**: login e controle de acesso
8. **Integração WhatsApp**: enviar lembrete de troca ao cliente (usando histórico para calcular próxima data)
9. **Busca avançada**: filtrar por período, tipo de óleo, indicação, etc.
10. **Impressão de comprovante**: recibo da troca para o cliente com histórico resumido

### Bibliotecas Auxiliares:
- **PCRE** (regex em C): para validação mais robusta
- **cJSON**: manipular JSON para exportações
- **libxlsxwriter**: gerar relatórios em Excel
- **PDFGen**: gerar relatórios em PDF

---

## TRATAMENTO DE ERROS

### Principais Verificações:

```c
// Conexão com banco de dados
if (sqlite3_open(db_path, &db) != SQLITE_OK) {
    MessageBox(NULL, "Erro ao conectar ao banco de dados!", "Erro", MB_ICONERROR);
    return -1;
}

// Execução de SQL
if (sqlite3_exec(db, sql, callback, data, &errmsg) != SQLITE_OK) {
    char msg[256];
    sprintf(msg, "Erro SQL: %s", errmsg);
    MessageBox(NULL, msg, "Erro", MB_ICONERROR);
    sqlite3_free(errmsg);
    return -1;
}

// Validação de placa
if (validar_placa(placa) == PLACA_INVALIDA) {
    MessageBox(hwnd, 
        "Placa inválida!\n\n"
        "Formatos aceitos:\n"
        "- Mercosul: ABC1D23\n"
        "- Brasil: ABC-1234", 
        "Validação", MB_ICONWARNING);
    SetFocus(GetDlgItem(hwnd, IDC_EDIT_PLACA));
    return;
}
```

---

## SEGURANÇA

### Prepared Statements (SQLite):
```c
// NUNCA concatenar strings diretamente no SQL!
// Usar prepared statements para evitar SQL injection:

sqlite3_stmt* stmt;
const char* sql = "INSERT INTO trocas_oleo (placa, tipo_oleo, telefone) VALUES (?, ?, ?)";

sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(stmt, 2, tipo_oleo, -1, SQLITE_TRANSIENT);
sqlite3_bind_text(stmt, 3, telefone, -1, SQLITE_TRANSIENT);

if (sqlite3_step(stmt) != SQLITE_DONE) {
    // erro
}

sqlite3_finalize(stmt);
```

---

## EXEMPLO DE README.md

```markdown
# 🚗 Sistema de Controle de Troca de Óleo

Sistema desktop em C para gerenciamento de trocas de óleo de veículos.

## 🛠️ Requisitos

- Windows 10/11
- SQLite3 (incluído)
- MinGW-GCC (para compilação)

## 📦 Instalação

1. Baixe o executável `TrocaOleo.exe`
2. Coloque na pasta `C:\TrocaOleo\`
3. Execute o programa
4. Configure o caminho do banco de dados no menu "Configurações"

## ⌨️ Atalho de Teclado

Configure em: Propriedades do atalho → Tecla de atalho → `Ctrl + Alt + T`

## 📊 Funcionalidades

- ✅ Cadastro de trocas de óleo
- ✅ Validação de placas Mercosul e Brasil
- ✅ Tipos de óleo configuráveis
- ✅ Busca por placa
- ✅ Edição e exclusão de registros
- ✅ Banco de dados SQLite local

## 🗂️ Estrutura do Banco

- Tabela: `trocas_oleo` (registros de trocas)
- Tabela: `tipos_oleo` (tipos configuráveis)
- Tabela: `configuracoes` (settings do sistema)

## 📝 Licença

MIT License
```

---

## CHECKLIST DE IMPLEMENTAÇÃO

### Fase 1 - Base (Essencial):
- [ ] Estrutura de pastas e arquivos
- [ ] Configurar SQLite no projeto
- [ ] Criar tabelas no banco de dados
- [ ] Implementar WinMain e janela principal
- [ ] Criar controles básicos (EDIT, BUTTON)

### Fase 2 - CRUD:
- [ ] Função de inserir (CREATE)
- [ ] Função de listar (READ)
- [ ] Função de atualizar (UPDATE)
- [ ] Função de deletar (DELETE)
- [ ] Popular ListView com dados

### Fase 3 - Validações:
- [ ] Validar placa Mercosul (regex)
- [ ] Validar placa Brasil (regex)
- [ ] Validar telefone (opcional)
- [ ] Validar campos obrigatórios

### Fase 4 - Configurações:
- [ ] Criar arquivo config.ini
- [ ] Dialog box de configurações
- [ ] Dialog box de tipos de óleo
- [ ] Salvar/carregar configurações

### Fase 5 - Polimento:
- [ ] Tratamento de erros robusto
- [ ] Mensagens de confirmação
- [ ] Ícone do programa
- [ ] Documentação (README)
- [ ] Testes de usabilidade

---

## RESUMO EXECUTIVO

**Objetivo**: Programa C com GUI Win32 nativa para controle de trocas de óleo **com sistema de histórico completo por veículo**.

**Componentes principais**:
1. Interface Win32 API (formulário + ListView + janela de histórico)
2. Banco SQLite3 (arquivo .db configurável)
3. Validação de placas (Mercosul/Brasil por regex)
4. CRUD completo (inserir, listar, editar, deletar)
5. **Sistema de histórico**: múltiplos registros por placa
6. Estatísticas por veículo (total trocas, intervalo médio, óleo preferido)
7. Sistema de configuração (config.ini + dialog boxes)
8. Tipos de óleo configuráveis pelo usuário

**Estrutura técnica**:
- 5 módulos em C: main, database, gui, validation, config
- Prepared statements para segurança
- Soft delete (ativo=0)
- Data automática no INSERT (timestamp atual)
- **VIEW SQL para última troca de cada veículo** (performance)
- Índices otimizados para queries de histórico

**Entregáveis**:
- Executável TrocaOleo.exe
- sqlite3.dll
- config.ini (gerado automaticamente)
- README.md com instruções

**Diferenciais**:
- Zero dependências externas (Win32 API nativa)
- Banco de dados portátil (arquivo .db)
- Configuração flexível do caminho do BD
- Atalho de teclado para acesso rápido
- Interface intuitiva e responsiva
- **Histórico completo**: rastreabilidade total de cada veículo
- **Relatórios analíticos**: estatísticas por veículo e gerais
- **Modo de visualização duplo**: todas as trocas OU resumo (última troca por veículo)

---

## NOTAS FINAIS PARA O COPILOT

- Use sempre **prepared statements** do SQLite para evitar SQL injection
- Implemente **soft delete** (campo `ativo`) ao invés de DELETE real
- A data da troca deve ser **automática** (timestamp do INSERT), mas editável depois
- O checkbox "Informar Telefone" controla se o campo telefone fica habilitado
- Radio buttons de tipos de óleo devem ser **criados dinamicamente** ao carregar o programa
- Use `CommCtrl` para controles avançados (ListView, DateTimePicker)
- Compile com `-lcomctl32` para suporte a controles comuns do Windows
- Teste a validação de placa com casos extremos (espaços, minúsculas, caracteres especiais)
- Implemente mutex/lock se houver acesso concorrente ao .db (SQLite suporta)
- Mantenha o código **modular** (um arquivo .c por funcionalidade)

**Boa sorte com a implementação! 🚀**

---

## ⚠️ NOTA CRÍTICA SOBRE PLACAS DUPLICADAS

### Comportamento do Sistema:
- ✅ **Placas duplicadas são PERMITIDAS** (histórico completo)
- ✅ Cada INSERT cria um **novo registro** independente
- ✅ **Não há validação** para evitar placa duplicada
- ✅ Um veículo (placa) pode ter **N registros** ao longo do tempo

### Por que isso funciona:
```
Cenário Real:
Cliente ABC1D23 volta 3 vezes:

1ª visita (05/06/2025): INSERT → ID=1, placa=ABC1D23, oleo=5W30
2ª visita (10/09/2025): INSERT → ID=2, placa=ABC1D23, oleo=Mineral
3ª visita (23/03/2026): INSERT → ID=3, placa=ABC1D23, oleo=5W30

Resultado no banco:
╔════╦══════════╦═══════════╦════════════╗
║ ID ║  Placa   ║   Óleo    ║    Data    ║
╠════╬══════════╬═══════════╬════════════╣
║ 3  ║ ABC1D23  ║ 5W30      ║ 23/03/2026 ║
║ 2  ║ ABC1D23  ║ Mineral   ║ 10/09/2025 ║
║ 1  ║ ABC1D23  ║ 5W30      ║ 05/06/2025 ║
╚════╩══════════╩═══════════╩════════════╝

Isso é CORRETO e DESEJADO! ✓
```

### O que NÃO fazer:
```c
// ❌ NÃO implementar esta verificação:
SELECT COUNT(*) FROM trocas_oleo WHERE placa = 'ABC1D23';
if (count > 0) {
    MessageBox("Placa já cadastrada!");
    return; // ❌ ERRADO! Não bloquear!
}
```

### Quando editar um registro:
```c
// ✓ Editar apenas o registro específico pelo ID:
UPDATE trocas_oleo 
SET tipo_oleo='Sintético', data_troca='2026-03-24' 
WHERE id=3;  // ← Edita APENAS este registro, não todos da placa ABC1D23

// ❌ NUNCA fazer:
UPDATE trocas_oleo 
SET tipo_oleo='Sintético' 
WHERE placa='ABC1D23';  // ← Isso editaria TODOS os 3 registros!
```

### Exclusão (soft delete):
```c
// ✓ Deletar apenas um registro:
UPDATE trocas_oleo SET ativo=0 WHERE id=3;

// Se quiser deletar TODOS os registros de uma placa (raro):
UPDATE trocas_oleo SET ativo=0 WHERE placa='ABC1D23';
// Mas normalmente você deleta só um registro por vez!
```

### Interface deve deixar claro:
1. **ListView** mostra TODAS as trocas (ou últimas, conforme filtro)
2. Ao selecionar uma linha, você edita **AQUELE registro específico** (pelo ID)
3. Ao clicar "Ver Histórico", mostra **todos os registros daquela placa**
4. Ao clicar "Excluir", remove **apenas o registro selecionado** (não todos da placa)

Esta é a arquitetura correta para um sistema de histórico! 🎯
