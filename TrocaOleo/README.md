# Sistema de Controle de Troca de Oleo

Aplicativo desktop em C (Win32 API) com SQLite para cadastro e historico de trocas de oleo.

## Requisitos

- Windows 10/11
- MinGW-w64 (gcc) ou MSVC
- sqlite3.dll disponivel em `lib/`

## Estrutura

- `src/main.c`: WinMain, WndProc e fluxo principal
- `src/database.c`: schema, CRUD e consultas de historico
- `src/gui.c`: criacao da interface Win32 e ListView
- `src/validation.c`: validacao de placa/telefone/data
- `src/config.c`: leitura e escrita do `config.ini`

## Banco de Dados

O sistema foi desenhado para historico completo:

- Placas duplicadas sao permitidas
- Cada troca gera novo registro
- Exclusao e soft delete (`ativo = 0`)
- Consulta de resumo por veiculo via view `ultimas_trocas`

## Compilacao (MinGW)

```bash
mingw32-make
```

## Executar

```bash
mingw32-make run
```

## Limpar objetos

```bash
mingw32-make clean
```

## Validacao de Placa

- Mercosul: `ABC1D23`
- Brasil antigo: `ABC-1234`

## Observacoes

- O menu de configuracao de tipos de oleo esta preparado no banco e na GUI dinamica, mas o dialogo grafico dedicado de manutencao ainda pode ser expandido.
- O botao de historico exibe estatisticas consolidadas por placa.
- Ao editar/excluir, a operacao sempre ocorre por `id` do registro selecionado.
