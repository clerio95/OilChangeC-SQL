# LGPD — Medidas do TrocaOleo

Referência das medidas de adequação à LGPD (auditoria de 2026-07-15). Parte é código
(já implementada), parte é operacional (checklist abaixo).

## Implementado no código (2026-07-17)

| Medida | Onde |
|---|---|
| Ledger de consentimento append-only (`consentimento_log`: evento, origem, versão do termo, data) | `database.c` — eventos `concedido`, `revogado`, `optout`, `optout_removido` |
| Revogação como evento: desmarcar consentimento preserva `data_consentimento` e carimba `data_revogacao` | `db_atualizar_troca` |
| Re-consentimento (novo aceite após revogação ou remoção de opt-out) gera carimbo novo — é o que reautoriza envio após um SAIR | `db_atualizar_troca` |
| Exclusão real: soft delete carimba `data_exclusao`; expurgo físico após 30 dias de carência | `db_deletar_troca`, `db_expurgar_dados_pessoais` |
| Retenção: telefone + dados de consentimento anonimizados após `retencao_meses` (config.ini, padrão 24; 0 desativa). Histórico do veículo (placa/óleo/data) permanece | `db_expurgar_dados_pessoais`, roda na inicialização |
| Lista de supressão `telefones_suprimidos`: opt-out sobrevive ao expurgo; novo cadastro do número herda o bloqueio | `db_inserir_troca`, `db_expurgar_dados_pessoais` |
| Vista `clientes_para_contato` com colunas explícitas (mínimo necessário) | `db_criar_tabelas` |
| Versão do termo de consentimento: `LGPD_VERSAO_TERMO` em `database.h` — **atualize junto com AVISO_PRIVACIDADE.md** | gravada em cada evento `concedido` |

O expurgo roda no banco local; a réplica de rede converge no próximo sync (backup integral).

## Checklist operacional (pendente — não é código)

1. **ACL do compartilhamento de rede**: restringir leitura/escrita da pasta do `dados.db`
   de rede à conta do operador e à conta que roda o oilnotify. Ninguém mais.
2. **Criptografia em repouso**: habilitar BitLocker (ou EFS na pasta) na máquina do
   balcão e no servidor do compartilhamento. Alternativa futura: migrar para SQLCipher.
3. **Aviso impresso no balcão**: imprimir `AVISO_PRIVACIDADE.md` preenchendo
   controlador/CNPJ/contato.
4. **Registro simplificado de tratamento** (Resolução CD/ANPD nº 2/2022, para ME/EPP):
   documentar — dados tratados (placa, telefone, datas), finalidade (lembrete de troca),
   base legal (consentimento, Art. 7º-I), retenção (24 meses), compartilhamento (nenhum),
   operador interno (módulo oilnotify/ZamOps, leitura da réplica, envio via WhatsApp).
5. **Transporte WhatsApp**: concluir migração do bridge não-oficial para a Meta Cloud API
   (transporte já existe no ZamOps; é configuração).

## Direitos do titular — como atender no balcão

- **Acesso/confirmação**: buscar pela placa e mostrar o histórico.
- **Correção**: editar o registro.
- **Revogação do aviso**: desmarcar "Cliente aceita aviso por WhatsApp" (fica registrado
  no ledger; a prova do consentimento anterior é preservada).
- **Não contatar**: marcar "Pediu para NAO contatar" (propaga para todas as trocas do
  telefone e sobrevive ao expurgo via lista de supressão).
- **Exclusão**: excluir o registro — apagamento físico automático em até 30 dias.
