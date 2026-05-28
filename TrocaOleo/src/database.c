#include "database.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static sqlite3 *g_db = NULL;

static int bind_like_filter(sqlite3_stmt *stmt, int idx, const char *filtro)
{
    char like[64];
    if (filtro == NULL || filtro[0] == '\0')
    {
        return sqlite3_bind_text(stmt, idx, "%", -1, SQLITE_STATIC);
    }

    snprintf(like, sizeof(like), "%%%s%%", filtro);
    return sqlite3_bind_text(stmt, idx, like, -1, SQLITE_TRANSIENT);
}

static char *dup_col_text(sqlite3_stmt *stmt, int col)
{
    const unsigned char *txt = sqlite3_column_text(stmt, col);
    size_t len;
    char *out;

    if (txt == NULL)
    {
        out = (char *)malloc(1);
        if (out != NULL)
        {
            out[0] = '\0';
        }
        return out;
    }

    len = strlen((const char *)txt);
    out = (char *)malloc(len + 1);
    if (out == NULL)
    {
        return NULL;
    }

    memcpy(out, txt, len + 1);
    return out;
}

static void preencher_troca_stmt(sqlite3_stmt *stmt, TrocaOleo *t)
{
    const unsigned char *s;

    memset(t, 0, sizeof(*t));
    t->id = sqlite3_column_int(stmt, 0);

    s = sqlite3_column_text(stmt, 1);
    if (s)
        snprintf(t->placa, sizeof(t->placa), "%s", (const char *)s);

    s = sqlite3_column_text(stmt, 2);
    if (s)
        snprintf(t->tipo_oleo, sizeof(t->tipo_oleo), "%s", (const char *)s);

    s = sqlite3_column_text(stmt, 3);
    if (s)
        snprintf(t->telefone, sizeof(t->telefone), "%s", (const char *)s);

    t->telefone_informado = sqlite3_column_int(stmt, 4);
    t->veio_indicacao     = sqlite3_column_int(stmt, 5);

    s = sqlite3_column_text(stmt, 6);
    if (s)
        snprintf(t->data_troca, sizeof(t->data_troca), "%s", (const char *)s);

    s = sqlite3_column_text(stmt, 7);
    if (s)
        snprintf(t->data_cadastro, sizeof(t->data_cadastro), "%s", (const char *)s);

    t->ativo                = sqlite3_column_int(stmt, 8);
    t->km_semanal_informado = sqlite3_column_int(stmt, 9);
    t->km_semanal           = sqlite3_column_int(stmt, 10);
    t->retorno_avisado      = sqlite3_column_int(stmt, 11);
}

static TrocaOleo *listar_trocas_por_sql(const char *sql, const char *filtro, int *count)
{
    sqlite3_stmt *stmt = NULL;
    TrocaOleo *lista = NULL;
    int capacidade = 0;
    int n = 0;
    int rc;

    if (count == NULL || g_db == NULL)
    {
        return NULL;
    }

    *count = 0;

    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        return NULL;
    }

    if (bind_like_filter(stmt, 1, filtro) != SQLITE_OK)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (n == capacidade)
        {
            int nova_cap = (capacidade == 0) ? 16 : capacidade * 2;
            TrocaOleo *novo = (TrocaOleo *)realloc(lista, (size_t)nova_cap * sizeof(TrocaOleo));
            if (novo == NULL)
            {
                free(lista);
                sqlite3_finalize(stmt);
                return NULL;
            }
            lista = novo;
            capacidade = nova_cap;
        }

        preencher_troca_stmt(stmt, &lista[n]);
        n++;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        free(lista);
        return NULL;
    }

    *count = n;
    return lista;
}

static int column_exists(const char *table, const char *column)
{
    char sql[256];
    sqlite3_stmt *stmt = NULL;
    int found = 0;

    snprintf(sql, sizeof(sql), "PRAGMA table_info(%s);", table);
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return 0;

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *col_name = (const char *)sqlite3_column_text(stmt, 1);
        if (col_name && strcmp(col_name, column) == 0)
        {
            found = 1;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

static void add_column_if_missing(const char *table, const char *column, const char *type_def)
{
    char sql[256];
    if (column_exists(table, column))
        return;
    snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD COLUMN %s %s;", table, column, type_def);
    sqlite3_exec(g_db, sql, NULL, NULL, NULL);
}

int db_init(const char *db_path, int criar_se_nao_existir)
{
    wchar_t wpath[MAX_PATH];
    DWORD attr;

    if (db_path == NULL)
        return -1;

    if (g_db != NULL)
        return 0;

    if (MultiByteToWideChar(CP_ACP, 0, db_path, -1, wpath, MAX_PATH) == 0)
        return -1;

    if (!criar_se_nao_existir)
    {
        attr = GetFileAttributesW(wpath);
        if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
            return -1;
    }

    if (sqlite3_open16(wpath, &g_db) != SQLITE_OK)
    {
        if (g_db != NULL)
        {
            sqlite3_close(g_db);
            g_db = NULL;
        }
        return -1;
    }

    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);
    return 0;
}

int db_criar_tabelas(void)
{
    const char *sql_schema =
        "CREATE TABLE IF NOT EXISTS trocas_oleo ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "placa TEXT NOT NULL,"
        "tipo_oleo TEXT NOT NULL,"
        "telefone TEXT,"
        "telefone_informado INTEGER DEFAULT 0,"
        "veio_indicacao INTEGER DEFAULT 0,"
        "data_troca TEXT NOT NULL,"
        "data_cadastro TEXT DEFAULT CURRENT_TIMESTAMP,"
        "ativo INTEGER DEFAULT 1,"
        "km_semanal_informado INTEGER DEFAULT 0,"
        "km_semanal INTEGER DEFAULT 0,"
        "retorno_avisado INTEGER DEFAULT 0"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_placa ON trocas_oleo(placa);"
        "CREATE INDEX IF NOT EXISTS idx_data_troca ON trocas_oleo(data_troca DESC);"
        "CREATE INDEX IF NOT EXISTS idx_placa_data ON trocas_oleo(placa, data_troca DESC);"

        "CREATE VIEW IF NOT EXISTS ultimas_trocas AS "
        "SELECT t1.* "
        "FROM trocas_oleo t1 "
        "INNER JOIN ("
        "  SELECT placa, MAX(data_troca) AS max_data "
        "  FROM trocas_oleo "
        "  WHERE ativo = 1 "
        "  GROUP BY placa"
        ") t2 ON t1.placa = t2.placa AND t1.data_troca = t2.max_data "
        "WHERE t1.ativo = 1;"

        "CREATE TABLE IF NOT EXISTS tipos_oleo ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "nome TEXT NOT NULL UNIQUE,"
        "ativo INTEGER DEFAULT 1"
        ");"

        "INSERT OR IGNORE INTO tipos_oleo (nome) VALUES "
        "('Mineral'),"
        "('Semissintetico'),"
        "('Sintetico'),"
        "('5W30'),"
        "('10W40');"

        "CREATE TABLE IF NOT EXISTS configuracoes ("
        "chave TEXT PRIMARY KEY,"
        "valor TEXT NOT NULL"
        ");"

        "INSERT OR IGNORE INTO configuracoes (chave, valor) VALUES "
        "('caminho_bd', 'C:\\\\TrocaOleo\\\\dados.db');";

    /* clientes_para_contato: rebuilt every startup so the definition stays current.
       Third-party script queries: SELECT * FROM clientes_para_contato WHERE oleo_vencido=1 */
    const char *sql_vista_contato =
        "DROP VIEW IF EXISTS clientes_para_contato;"
        "CREATE VIEW clientes_para_contato AS "
        "SELECT t.*, "
        "  CAST(julianday('now') - julianday(t.data_troca) AS INTEGER) AS dias_desde_troca, "
        "  CASE "
        "    WHEN t.km_semanal_informado = 1 AND t.km_semanal > 0 "
        "         AND CAST((julianday('now') - julianday(t.data_troca)) / 7.0 * t.km_semanal AS INTEGER) >= 10000 "
        "    THEN 1 "
        "    WHEN (t.km_semanal_informado = 0 OR t.km_semanal = 0) "
        "         AND CAST(julianday('now') - julianday(t.data_troca) AS INTEGER) >= 180 "
        "    THEN 1 "
        "    ELSE 0 "
        "  END AS oleo_vencido "
        "FROM ultimas_trocas t "
        "WHERE t.telefone_informado = 1 "
        "  AND t.retorno_avisado = 0;";

    char *err = NULL;

    if (g_db == NULL)
        return -1;

    if (sqlite3_exec(g_db, sql_schema, NULL, NULL, &err) != SQLITE_OK)
    {
        if (err)
            sqlite3_free(err);
        return -1;
    }

    /* Migrate existing databases that predate these columns */
    add_column_if_missing("trocas_oleo", "km_semanal_informado", "INTEGER DEFAULT 0");
    add_column_if_missing("trocas_oleo", "km_semanal",           "INTEGER DEFAULT 0");
    add_column_if_missing("trocas_oleo", "retorno_avisado",      "INTEGER DEFAULT 0");

    /* Rebuild the contact view after migration so column references are valid */
    if (sqlite3_exec(g_db, sql_vista_contato, NULL, NULL, &err) != SQLITE_OK)
    {
        if (err)
            sqlite3_free(err);
    }

    return 0;
}

int db_inserir_troca(const TrocaOleo *troca)
{
    const char *sql =
        "INSERT INTO trocas_oleo "
        "(placa, tipo_oleo, telefone, telefone_informado, veio_indicacao, data_troca, "
        "km_semanal_informado, km_semanal) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL || troca == NULL)
        return -1;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, troca->placa, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, troca->tipo_oleo, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, troca->telefone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, troca->telefone_informado);
    sqlite3_bind_int(stmt, 5, troca->veio_indicacao);
    sqlite3_bind_text(stmt, 6, troca->data_troca, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, troca->km_semanal_informado);
    sqlite3_bind_int(stmt, 8, troca->km_semanal);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

TrocaOleo *db_listar_trocas(const char *filtro_placa, int *count)
{
    const char *sql =
        "SELECT id, placa, tipo_oleo, telefone, telefone_informado, veio_indicacao, "
        "data_troca, data_cadastro, ativo, km_semanal_informado, km_semanal, retorno_avisado "
        "FROM trocas_oleo "
        "WHERE ativo = 1 AND placa LIKE ? "
        "ORDER BY data_troca DESC;";

    return listar_trocas_por_sql(sql, filtro_placa, count);
}

TrocaOleo *db_listar_ultimas_trocas(const char *filtro_placa, int *count)
{
    const char *sql =
        "SELECT id, placa, tipo_oleo, telefone, telefone_informado, veio_indicacao, "
        "data_troca, data_cadastro, ativo, km_semanal_informado, km_semanal, retorno_avisado "
        "FROM ultimas_trocas "
        "WHERE placa LIKE ? "
        "ORDER BY data_troca DESC;";

    return listar_trocas_por_sql(sql, filtro_placa, count);
}

int db_atualizar_troca(int id, const TrocaOleo *troca)
{
    const char *sql =
        "UPDATE trocas_oleo "
        "SET placa=?, tipo_oleo=?, telefone=?, telefone_informado=?, veio_indicacao=?, "
        "data_troca=?, km_semanal_informado=?, km_semanal=? "
        "WHERE id=?;";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL || troca == NULL)
        return -1;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_text(stmt, 1, troca->placa, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, troca->tipo_oleo, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, troca->telefone, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, troca->telefone_informado);
    sqlite3_bind_int(stmt, 5, troca->veio_indicacao);
    sqlite3_bind_text(stmt, 6, troca->data_troca, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, troca->km_semanal_informado);
    sqlite3_bind_int(stmt, 8, troca->km_semanal);
    sqlite3_bind_int(stmt, 9, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_deletar_troca(int id)
{
    const char *sql = "UPDATE trocas_oleo SET ativo=0 WHERE id=?;";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

TrocaOleo *db_buscar_troca_por_id(int id)
{
    const char *sql =
        "SELECT id, placa, tipo_oleo, telefone, telefone_informado, veio_indicacao, "
        "data_troca, data_cadastro, ativo, km_semanal_informado, km_semanal, retorno_avisado "
        "FROM trocas_oleo WHERE id = ?;";
    sqlite3_stmt *stmt = NULL;
    TrocaOleo *out;

    if (g_db == NULL)
    {
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    out = (TrocaOleo *)malloc(sizeof(TrocaOleo));
    if (out == NULL)
    {
        sqlite3_finalize(stmt);
        return NULL;
    }

    preencher_troca_stmt(stmt, out);
    sqlite3_finalize(stmt);
    return out;
}

TrocaOleo *db_historico_por_placa(const char *placa, int *count)
{
    const char *sql =
        "SELECT id, placa, tipo_oleo, telefone, telefone_informado, veio_indicacao, "
        "data_troca, data_cadastro, ativo, km_semanal_informado, km_semanal, retorno_avisado "
        "FROM trocas_oleo WHERE ativo=1 AND placa=? ORDER BY data_troca DESC;";
    sqlite3_stmt *stmt = NULL;
    TrocaOleo *lista = NULL;
    int capacidade = 0;
    int n = 0;
    int rc;

    if (count == NULL || placa == NULL || g_db == NULL)
    {
        return NULL;
    }

    *count = 0;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (n == capacidade)
        {
            int nova_cap = (capacidade == 0) ? 8 : capacidade * 2;
            TrocaOleo *novo = (TrocaOleo *)realloc(lista, (size_t)nova_cap * sizeof(TrocaOleo));
            if (novo == NULL)
            {
                free(lista);
                sqlite3_finalize(stmt);
                return NULL;
            }
            lista = novo;
            capacidade = nova_cap;
        }
        preencher_troca_stmt(stmt, &lista[n]);
        n++;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        free(lista);
        return NULL;
    }

    *count = n;
    return lista;
}

int db_contar_trocas_por_placa(const char *placa)
{
    const char *sql = "SELECT COUNT(*) FROM trocas_oleo WHERE placa=? AND ativo=1;";
    sqlite3_stmt *stmt = NULL;
    int total = 0;

    if (g_db == NULL || placa == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}

char *db_tipo_oleo_mais_usado(const char *placa)
{
    const char *sql =
        "SELECT tipo_oleo, COUNT(*) AS vezes "
        "FROM trocas_oleo "
        "WHERE placa=? AND ativo=1 "
        "GROUP BY tipo_oleo "
        "ORDER BY vezes DESC, tipo_oleo ASC "
        "LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    char *out = NULL;

    if (g_db == NULL || placa == NULL)
    {
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out = dup_col_text(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return out;
}

int db_intervalo_medio_dias(const char *placa)
{
    const char *sql =
        "WITH trocas_ordenadas AS ("
        "  SELECT data_troca, LAG(data_troca) OVER (ORDER BY data_troca) AS troca_anterior "
        "  FROM trocas_oleo "
        "  WHERE placa = ? AND ativo = 1"
        ") "
        "SELECT AVG(julianday(data_troca) - julianday(troca_anterior)) "
        "FROM trocas_ordenadas "
        "WHERE troca_anterior IS NOT NULL;";
    sqlite3_stmt *stmt = NULL;
    int media = 0;

    if (g_db == NULL || placa == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL)
    {
        media = (int)(sqlite3_column_double(stmt, 0) + 0.5);
    }

    sqlite3_finalize(stmt);
    return media;
}

char *db_data_primeira_troca(const char *placa)
{
    const char *sql =
        "SELECT data_troca FROM trocas_oleo "
        "WHERE placa=? AND ativo=1 ORDER BY data_troca ASC LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    char *out = NULL;

    if (g_db == NULL || placa == NULL)
    {
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out = dup_col_text(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return out;
}

char *db_data_ultima_troca(const char *placa)
{
    const char *sql =
        "SELECT data_troca FROM trocas_oleo "
        "WHERE placa=? AND ativo=1 ORDER BY data_troca DESC LIMIT 1;";
    sqlite3_stmt *stmt = NULL;
    char *out = NULL;

    if (g_db == NULL || placa == NULL)
    {
        return NULL;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    sqlite3_bind_text(stmt, 1, placa, -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        out = dup_col_text(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return out;
}

int db_total_veiculos_cadastrados(void)
{
    const char *sql = "SELECT COUNT(DISTINCT placa) FROM trocas_oleo WHERE ativo=1;";
    sqlite3_stmt *stmt = NULL;
    int total = 0;

    if (g_db == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}

int db_total_trocas_realizadas(void)
{
    const char *sql = "SELECT COUNT(*) FROM trocas_oleo WHERE ativo=1;";
    sqlite3_stmt *stmt = NULL;
    int total = 0;

    if (g_db == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        total = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return total;
}

char **db_listar_placas_unicas(int *count)
{
    const char *sql = "SELECT DISTINCT placa FROM trocas_oleo WHERE ativo=1 ORDER BY placa;";
    sqlite3_stmt *stmt = NULL;
    char **lista = NULL;
    int capacidade = 0;
    int n = 0;
    int rc;

    if (count == NULL || g_db == NULL)
    {
        return NULL;
    }

    *count = 0;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        if (n == capacidade)
        {
            int nova_cap = (capacidade == 0) ? 16 : capacidade * 2;
            char **novo = (char **)realloc(lista, (size_t)nova_cap * sizeof(char *));
            if (novo == NULL)
            {
                db_liberar_lista_strings(lista, n);
                sqlite3_finalize(stmt);
                return NULL;
            }
            lista = novo;
            capacidade = nova_cap;
        }

        lista[n] = dup_col_text(stmt, 0);
        if (lista[n] == NULL)
        {
            db_liberar_lista_strings(lista, n);
            sqlite3_finalize(stmt);
            return NULL;
        }
        n++;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        db_liberar_lista_strings(lista, n);
        return NULL;
    }

    *count = n;
    return lista;
}

TipoOleo *db_listar_tipos_oleo(int *count)
{
    const char *sql = "SELECT id, nome, ativo FROM tipos_oleo WHERE ativo=1 ORDER BY nome;";
    sqlite3_stmt *stmt = NULL;
    TipoOleo *lista = NULL;
    int capacidade = 0;
    int n = 0;
    int rc;

    if (count == NULL || g_db == NULL)
    {
        return NULL;
    }

    *count = 0;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return NULL;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char *nome;

        if (n == capacidade)
        {
            int nova_cap = (capacidade == 0) ? 8 : capacidade * 2;
            TipoOleo *novo = (TipoOleo *)realloc(lista, (size_t)nova_cap * sizeof(TipoOleo));
            if (novo == NULL)
            {
                free(lista);
                sqlite3_finalize(stmt);
                return NULL;
            }
            lista = novo;
            capacidade = nova_cap;
        }

        memset(&lista[n], 0, sizeof(TipoOleo));
        lista[n].id = sqlite3_column_int(stmt, 0);
        nome = sqlite3_column_text(stmt, 1);
        if (nome)
            snprintf(lista[n].nome, sizeof(lista[n].nome), "%s", (const char *)nome);
        lista[n].ativo = sqlite3_column_int(stmt, 2);
        n++;
    }

    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE)
    {
        free(lista);
        return NULL;
    }

    *count = n;
    return lista;
}

int db_adicionar_tipo_oleo(const char *nome)
{
    const char *sql = "INSERT OR IGNORE INTO tipos_oleo (nome, ativo) VALUES (?, 1);";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL || nome == NULL || nome[0] == '\0')
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, nome, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_remover_tipo_oleo(int id)
{
    const char *sql = "UPDATE tipos_oleo SET ativo=0 WHERE id=?;";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL)
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_remover_tipo_oleo_por_nome(const char *nome)
{
    const char *sql = "UPDATE tipos_oleo SET ativo=0 WHERE nome=? AND ativo=1;";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL || nome == NULL || nome[0] == '\0')
    {
        return -1;
    }

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, nome, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_sincronizar_para_rede(const char *network_path)
{
    wchar_t wpath[MAX_PATH];
    sqlite3 *dest = NULL;
    sqlite3_backup *backup;
    int rc;

    if (network_path == NULL || network_path[0] == '\0' || g_db == NULL)
        return -1;

    if (MultiByteToWideChar(CP_ACP, 0, network_path, -1, wpath, MAX_PATH) == 0)
        return -1;

    if (sqlite3_open16(wpath, &dest) != SQLITE_OK)
    {
        if (dest)
            sqlite3_close(dest);
        return -1;
    }

    backup = sqlite3_backup_init(dest, "main", g_db, "main");
    if (backup == NULL)
    {
        sqlite3_close(dest);
        return -1;
    }

    rc = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(dest);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_puxar_retorno_avisado(const char *network_path)
{
    wchar_t wpath[MAX_PATH];
    sqlite3 *rede = NULL;
    sqlite3_stmt *sel = NULL;
    sqlite3_stmt *upd = NULL;
    DWORD attr;

    if (network_path == NULL || network_path[0] == '\0' || g_db == NULL)
        return -1;

    if (MultiByteToWideChar(CP_ACP, 0, network_path, -1, wpath, MAX_PATH) == 0)
        return -1;

    attr = GetFileAttributesW(wpath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
        return -1;

    if (sqlite3_open16(wpath, &rede) != SQLITE_OK)
    {
        if (rede)
            sqlite3_close(rede);
        return -1;
    }

    if (sqlite3_prepare_v2(g_db,
            "UPDATE trocas_oleo SET retorno_avisado=1 WHERE id=? AND retorno_avisado=0;",
            -1, &upd, NULL) != SQLITE_OK)
    {
        sqlite3_close(rede);
        return -1;
    }

    if (sqlite3_prepare_v2(rede,
            "SELECT id FROM trocas_oleo WHERE retorno_avisado=1;",
            -1, &sel, NULL) == SQLITE_OK)
    {
        while (sqlite3_step(sel) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(sel, 0);
            sqlite3_bind_int(upd, 1, id);
            sqlite3_step(upd);
            sqlite3_reset(upd);
        }
        sqlite3_finalize(sel);
    }

    sqlite3_finalize(upd);
    sqlite3_close(rede);
    return 0;
}

int db_bootstrap_da_rede(const char *network_path)
{
    wchar_t wpath[MAX_PATH];
    sqlite3 *src = NULL;
    sqlite3_backup *backup;
    DWORD attr;
    int rc;

    if (network_path == NULL || network_path[0] == '\0' || g_db == NULL)
        return -1;

    if (MultiByteToWideChar(CP_ACP, 0, network_path, -1, wpath, MAX_PATH) == 0)
        return -1;

    attr = GetFileAttributesW(wpath);
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
        return -1;

    if (sqlite3_open16(wpath, &src) != SQLITE_OK)
    {
        if (src)
            sqlite3_close(src);
        return -1;
    }

    /* Copy network → local (g_db is the local open connection) */
    backup = sqlite3_backup_init(g_db, "main", src, "main");
    if (backup == NULL)
    {
        sqlite3_close(src);
        return -1;
    }

    rc = sqlite3_backup_step(backup, -1);
    sqlite3_backup_finish(backup);
    sqlite3_close(src);
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int db_marcar_retorno_avisado(int id)
{
    const char *sql = "UPDATE trocas_oleo SET retorno_avisado=1 WHERE id=?;";
    sqlite3_stmt *stmt = NULL;

    if (g_db == NULL)
        return -1;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return -1;

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_DONE)
    {
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

void db_fechar(void)
{
    if (g_db != NULL)
    {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

void db_liberar_trocas(TrocaOleo *trocas)
{
    free(trocas);
}

void db_liberar_tipos(TipoOleo *tipos)
{
    free(tipos);
}

void db_liberar_lista_strings(char **lista, int count)
{
    int i;
    if (lista == NULL)
    {
        return;
    }

    for (i = 0; i < count; i++)
    {
        free(lista[i]);
    }
    free(lista);
}
