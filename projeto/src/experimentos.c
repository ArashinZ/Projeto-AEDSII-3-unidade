/*
 * experimentos.c - Testes automatizados de desempenho (Parte 3)
 *
 * Insere N registros, busca N mistos (50% presentes / 50% ausentes)
 * e mede tempo com e sem Bloom para 3 tamanhos de conjunto.
 *
 * gcc experimentos.c hash.c Fbloom.c -lm -o experimentos
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
 
/* Busca direto na hash, sem filtrar pelo Bloom */
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
 
/* Filtra pelo Bloom antes de ir a hash */
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
            if (bloom_consultar(bf, u) != 0) {
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
 
    printf("\n=============================================================\n");
    printf("%-10s %-16s %-16s %-14s\n",
           "Qtd", "Sem Bloom (ms)", "Com Bloom (ms)", "Falsos Pos (%%)");
    printf("%-10s %-16s %-16s %-14s\n",
           "----------","----------------","----------------","--------------");
    for (int i = 0; i < 3; i++)
        printf("%-10d %-16.4f %-16.4f %-14.4f\n",
               res[i].n, res[i].t_sem, res[i].t_com, res[i].fp_pct);
 
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
 
    return EXIT_SUCCESS;
}
 
