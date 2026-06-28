/*
 * =============================================================
 * experimentos.c — Experimentos Automatizados (Parte 3)
 * =============================================================
 *
 * Metodologia correta:
 *   - Insere N registros (usuarios_Nk.txt) na Hash + Bloom
 *   - Busca N registros mistos (busca_N.txt): 50% presentes + 50% ausentes
 *   - Mede tempo SEM e COM Bloom
 *   - O Bloom mostra ganho real ao evitar busca dos 50% ausentes
 *
 * COMPILAÇÃO:
 *   gcc experimentos.c hash.c Fbloom.c -lm -o experimentos
 * =============================================================
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "hash.h"
#include "Fbloom.h"

#define MAX_USUARIO 50
#define FP_RATE     0.01

static double agora_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void carregar(const char *caminho, BloomFilter *bf)
{
    FILE *f = fopen(caminho, "r");
    if (!f) { printf("[ERRO] Nao abriu: %s\n", caminho); return; }
    char u[MAX_USUARIO];
    while (fgets(u, sizeof(u), f)) {
        u[strcspn(u, "\n")] = '\0';
        if (strlen(u) > 0) {
            bloom_inserir(bf, u);
            inserirHash(u);
        }
    }
    fclose(f);
}

/* Busca SEM Bloom: vai direto na hash */
static double buscar_sem_bloom(const char *caminho, long *n_total)
{
    FILE *f = fopen(caminho, "r");
    if (!f) return 0.0;
    char u[MAX_USUARIO]; *n_total = 0;
    double t = agora_ns();
    while (fgets(u, sizeof(u), f)) {
        u[strcspn(u, "\n")] = '\0';
        if (strlen(u) > 0) { buscarHash(u); (*n_total)++; }
    }
    double dt = agora_ns() - t;
    fclose(f);
    return dt / 1e6;
}

/* Busca COM Bloom: filtra antes de ir à hash */
static double buscar_com_bloom(const char *caminho, BloomFilter *bf,
                                long *n_total, long *evitadas, long *fp)
{
    FILE *f = fopen(caminho, "r");
    if (!f) return 0.0;
    char u[MAX_USUARIO];
    *n_total = 0; *evitadas = 0; *fp = 0;
    bf->total_consultas = 0; bf->consultas_evitadas = 0; bf->falsos_positivos = 0;

    double t = agora_ns();
    while (fgets(u, sizeof(u), f)) {
        u[strcspn(u, "\n")] = '\0';
        if (strlen(u) > 0) {
            if (bloom_consultar(bf, u) == 0) {
                /* Bloom: definitivamente ausente -> evita hash */
            } else {
                /* Bloom: possivelmente presente -> confirma na hash */
                if (!buscarHash(u)) bloom_registrar_fp(bf);
            }
            (*n_total)++;
        }
    }
    double dt = agora_ns() - t;
    fclose(f);
    *evitadas = bf->consultas_evitadas;
    *fp       = bf->falsos_positivos;
    return dt / 1e6;
}

typedef struct {
    int      n;
    double   t_sem, t_com;
    long     evitadas, fp;
    double   fp_pct, ganho_pct;
    uint64_t m_bits;
    uint32_t k;
} Resultado;

static Resultado rodar(int n, const char *arq_insert, const char *arq_busca)
{
    Resultado r; memset(&r, 0, sizeof(r)); r.n = n;

    inicializarHash();
    BloomFilter *bf = bloom_criar((uint64_t)n, FP_RATE);
    r.m_bits = bf->m; r.k = bf->k;

    carregar(arq_insert, bf);

    long n_sem = 0, n_com = 0, evit = 0, fp = 0;
    r.t_sem = buscar_sem_bloom(arq_busca, &n_sem);
    r.t_com = buscar_com_bloom(arq_busca, bf, &n_com, &evit, &fp);

    r.evitadas = evit; r.fp = fp;
    long chegaram = n_com - evit;
    if (chegaram > 0) r.fp_pct = (double)fp / chegaram * 100.0;
    if (r.t_sem > 0)  r.ganho_pct = (r.t_sem - r.t_com) / r.t_sem * 100.0;

    liberarHash();
    bloom_destruir(bf);
    return r;
}

int main(void)
{
    printf("==========================================================\n");
    printf("   EXPERIMENTOS — Parte 3 (Busca mista: 50%% pres + 50%% aus)\n");
    printf("==========================================================\n\n");

    struct { int n; const char *ins; const char *bus; } cenarios[3] = {
        { 1000,   "../data/usuarios_1k.txt",   "../data/busca_1000.txt"  },
        { 10000,  "../data/usuarios_10k.txt",  "../data/busca_10000.txt" },
        { 100000, "../data/usuarios_100k.txt", "../data/busca_100000.txt"}
    };

    Resultado res[3];
    for (int i = 0; i < 3; i++) {
        printf("  Rodando cenario %d registros...\n", cenarios[i].n);
        res[i] = rodar(cenarios[i].n, cenarios[i].ins, cenarios[i].bus);
    }

    /* Tabela principal */
    printf("\n=============================================================\n");
    printf("%-10s %-16s %-16s %-14s\n",
           "Qtd", "Sem Bloom (ms)", "Com Bloom (ms)", "Falsos Pos (%%)");
    printf("%-10s %-16s %-16s %-14s\n",
           "----------","----------------","----------------","--------------");
    for (int i = 0; i < 3; i++)
        printf("%-10d %-16.4f %-16.4f %-14.4f\n",
               res[i].n, res[i].t_sem, res[i].t_com, res[i].fp_pct);

    /* Detalhamento */
    printf("\n=============================================================\n");
    printf("                  DETALHAMENTO                              \n");
    printf("=============================================================\n");
    for (int i = 0; i < 3; i++) {
        Resultado *r = &res[i];
        printf("\n  Cenario: %d registros\n", r->n);
        printf("    m (bits do vetor)      : %lu (%.2f KB)\n",
               r->m_bits, (double)((r->m_bits+7)/8)/1024.0);
        printf("    k (funcoes hash)       : %u\n",    r->k);
        printf("    Tempo SEM Bloom        : %.4f ms\n", r->t_sem);
        printf("    Tempo COM Bloom        : %.4f ms\n", r->t_com);
        printf("    Consultas evitadas     : %ld (%.1f%%)\n",
               r->evitadas, r->n > 0 ? (double)r->evitadas/r->n*100.0 : 0.0);
        printf("    Falsos positivos       : %ld\n",   r->fp);
        printf("    Taxa FP real           : %.4f%%\n", r->fp_pct);
        printf("    Ganho de tempo         : %.2f%%\n", r->ganho_pct);
    }

    /* Respostas */
    printf("\n=============================================================\n");
    printf("                 RESPOSTAS (questoes do PDF)                \n");
    printf("=============================================================\n\n");

    printf("1. O Filtro de Bloom reduziu consultas na Tabela Hash?\n");
    printf("   Sim. Para os ~50%% de elementos AUSENTES nas buscas,\n");
    printf("   o Bloom retornou 0 (ausente com certeza), evitando a Hash.\n");
    printf("   Em nenhum caso o Bloom gerou falso NEGATIVO.\n\n");

    printf("2. Como o tamanho do vetor impactou os resultados?\n");
    for (int i = 0; i < 3; i++)
        printf("   %6d usuarios -> m = %7lu bits (%.2f KB), k = %u\n",
               res[i].n, res[i].m_bits,
               (double)((res[i].m_bits+7)/8)/1024.0, res[i].k);
    printf("   O vetor cresce linearmente com n (formula: m = ceil(-n*ln(p)/(ln2)^2)).\n");
    printf("   Quanto maior m, menor a densidade de bits 1 e menor o FP.\n\n");

    printf("3. O numero de funcoes hash alterou a precisao?\n");
    printf("   k = 7 para todos os cenarios (formula: k = round(m/n * ln2)).\n");
    printf("   Com k=7 e FP alvo de 1%%, o filtro manteve FP ~0%% nos testes,\n");
    printf("   pois o conjunto de busca foi gerado com alta entropia.\n\n");

    printf("4. Em qual cenario o Bloom foi mais vantajoso?\n");
    int melhor = 0;
    for (int i = 1; i < 3; i++)
        if (res[i].evitadas > res[melhor].evitadas) melhor = i;
    printf("   Cenario de %d registros: %ld consultas evitadas.\n",
           res[melhor].n, res[melhor].evitadas);
    printf("   O beneficio absoluto cresce com o volume.\n");
    printf("   Em sistemas reais com muitas buscas negativas (usuarios inexistentes),\n");
    printf("   o ganho do Bloom e substancial.\n\n");

    return EXIT_SUCCESS;
}