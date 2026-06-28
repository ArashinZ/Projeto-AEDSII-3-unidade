/*
 * principal.c - Sistema de Verificacao de Cadastro de Usuarios
 *
 * Integra Filtro de Bloom (Fbloom.c) com Tabela Hash (hash.c).
 * Fluxo de consulta: bloom primeiro, hash so se necessario.
 *
 * gcc principal.c hash.c Fbloom.c -lm -o sistema
 */
 
#define _POSIX_C_SOURCE 199309L
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
 
#include "hash.h"
#include "Fbloom.h"
 
#define CAPACIDADE_INICIAL 100000
#define FP_RATE            BLOOM_FP_PADRAO
#define MAX_USUARIO        50
 
static BloomFilter *bloom = NULL;
static uint64_t total_elementos   = 0;
static uint64_t total_consultas_g = 0;
static double   tempo_total_ns    = 0;
 
static double agora_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}
 
static void sistema_inserir(const char *usuario)
{
    bloom_inserir(bloom, usuario);
    inserirHash(usuario);
    total_elementos++;
}
 
/* Fluxo: bloom -> hash (so se bloom retornar 1) */
static int sistema_consultar(const char *usuario)
{
    double t_inicio = agora_ns();
    total_consultas_g++;
 
    if (bloom_consultar(bloom, usuario) == 0) {
        tempo_total_ns += agora_ns() - t_inicio;
        return 0;
    }
 
    int hash_resp = buscarHash(usuario);
    if (!hash_resp) bloom_registrar_fp(bloom);
 
    tempo_total_ns += agora_ns() - t_inicio;
    return hash_resp;
}
 
static void sistema_carregar_arquivo(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) {
        printf("[ERRO] Nao foi possivel abrir: %s\n", caminho);
        return;
    }
 
    char usuario[MAX_USUARIO];
    int  count = 0;
    while (fgets(usuario, sizeof(usuario), arq)) {
        usuario[strcspn(usuario, "\n")] = '\0';
        if (strlen(usuario) > 0) {
            sistema_inserir(usuario);
            count++;
        }
    }
    fclose(arq);
    printf("[OK] %d usuarios carregados de '%s'\n", count, caminho);
}
 
static void sistema_imprimir_estatisticas(void)
{
    printf("\n========================================\n");
    printf("         ESTATISTICAS DO SISTEMA        \n");
    printf("========================================\n");
    printf("Elementos armazenados      : %lu\n", total_elementos);
    printf("Consultas realizadas       : %lu\n", total_consultas_g);
    printf("Consultas evitadas (Bloom) : %lu\n", bloom->consultas_evitadas);
    printf("Falsos positivos           : %lu\n", bloom->falsos_positivos);
    printf("Taxa FP real               : %.4f%%\n", bloom_taxa_fp_real(bloom));
    if (total_consultas_g > 0)
        printf("Tempo medio por consulta   : %.2f ns\n",
               tempo_total_ns / (double)total_consultas_g);
    else
        printf("Tempo medio por consulta   : N/A\n");
    bloom_imprimir_info(bloom);
}
 
/* Baseline: busca sem Bloom */
static void experimento_sem_bloom(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) { printf("[ERRO] Arquivo nao encontrado: %s\n", caminho); return; }
 
    char     usuario[MAX_USUARIO];
    uint64_t buscas   = 0;
    double   t_inicio = agora_ns();
 
    while (fgets(usuario, sizeof(usuario), arq)) {
        usuario[strcspn(usuario, "\n")] = '\0';
        if (strlen(usuario) > 0) { buscarHash(usuario); buscas++; }
    }
 
    double t_total = agora_ns() - t_inicio;
    fclose(arq);
 
    printf("\n--- Experimento SEM Bloom (%s) ---\n", caminho);
    printf("Registros buscados : %lu\n", buscas);
    printf("Tempo total        : %.2f ms\n", t_total / 1e6);
    if (buscas > 0)
        printf("Tempo medio        : %.2f ns/busca\n", t_total / (double)buscas);
}
 
static void experimento_com_bloom(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) { printf("[ERRO] Arquivo nao encontrado: %s\n", caminho); return; }
 
    char     usuario[MAX_USUARIO];
    uint64_t buscas   = 0;
    double   t_inicio = agora_ns();
 
    while (fgets(usuario, sizeof(usuario), arq)) {
        usuario[strcspn(usuario, "\n")] = '\0';
        if (strlen(usuario) > 0) {
            if (bloom_consultar(bloom, usuario))
                buscarHash(usuario);
            buscas++;
        }
    }
 
    double t_total = agora_ns() - t_inicio;
    fclose(arq);
 
    printf("\n--- Experimento COM Bloom (%s) ---\n", caminho);
    printf("Registros buscados : %lu\n", buscas);
    printf("Tempo total        : %.2f ms\n", t_total / 1e6);
    if (buscas > 0)
        printf("Tempo medio        : %.2f ns/busca\n", t_total / (double)buscas);
}
 
static void imprimir_menu(void)
{
    printf("\n========================================\n");
    printf("  Sistema de Verificacao de Cadastro\n");
    printf("========================================\n");
    printf("  1. INSERIR usuario\n");
    printf("  2. CONSULTAR usuario\n");
    printf("  3. ESTATISTICAS\n");
    printf("  4. CARREGAR arquivo (lote)\n");
    printf("  5. EXPERIMENTO sem Bloom vs com Bloom\n");
    printf("  0. SAIR\n");
    printf("----------------------------------------\n");
    printf("Opcao: ");
}
 
int main(void)
{
    inicializarHash();
 
    bloom = bloom_criar(CAPACIDADE_INICIAL, FP_RATE);
    if (!bloom) {
        fprintf(stderr, "[ERRO FATAL] Nao foi possivel criar o Filtro de Bloom.\n");
        return EXIT_FAILURE;
    }
 
    printf("Sistema iniciado.\n");
    printf("Bloom: m=%lu bits | k=%u funcoes hash | FP alvo=%.2f%%\n",
           bloom->m, bloom->k, bloom->fp_alvo * 100.0);
 
    int  opcao;
    char usuario[MAX_USUARIO];
    char caminho[256];
 
    do {
        imprimir_menu();
        if (scanf("%d", &opcao) != 1) break;
        getchar();
 
        switch (opcao) {
        case 1:
            printf("Nome do usuario: ");
            if (fgets(usuario, sizeof(usuario), stdin)) {
                usuario[strcspn(usuario, "\n")] = '\0';
                sistema_inserir(usuario);
                printf("[OK] Usuario '%s' inserido.\n", usuario);
            }
            break;
 
        case 2:
            printf("Nome do usuario: ");
            if (fgets(usuario, sizeof(usuario), stdin)) {
                usuario[strcspn(usuario, "\n")] = '\0';
                int encontrado = sistema_consultar(usuario);
                printf("-> %s\n", encontrado ? "Usuario encontrado" : "Usuario inexistente");
            }
            break;
 
        case 3:
            sistema_imprimir_estatisticas();
            break;
 
        case 4:
            printf("Caminho do arquivo: ");
            if (fgets(caminho, sizeof(caminho), stdin)) {
                caminho[strcspn(caminho, "\n")] = '\0';
                sistema_carregar_arquivo(caminho);
            }
            break;
 
        case 5:
            printf("Caminho do arquivo de teste: ");
            if (fgets(caminho, sizeof(caminho), stdin)) {
                caminho[strcspn(caminho, "\n")] = '\0';
                experimento_sem_bloom(caminho);
                experimento_com_bloom(caminho);
            }
            break;
 
        case 0:
            printf("Encerrando o sistema.\n");
            break;
 
        default:
            printf("[AVISO] Opcao invalida.\n");
        }
    } while (opcao != 0);
 
    liberarHash();
    bloom_destruir(bloom);
    return EXIT_SUCCESS;
}
 
