/*
 * =============================================================
 * principal.c — Sistema de Verificação de Cadastro de Usuários
 * =============================================================
 *
 * Integra o Filtro de Bloom (Fbloom.c) com a Tabela Hash (hash.c).
 *
 * FLUXO DE CONSULTA (RF02):
 *   1. bloom_consultar() — se retornar 0: elemento AUSENTE, fim.
 *   2. Se retornar 1: buscarHash().
 *      - Hash confirma presença → "Usuário encontrado".
 *      - Hash nega presença     → falso positivo do Bloom,
 *        chamar bloom_registrar_fp(), retornar "Usuário inexistente".
 *
 * COMPILAÇÃO:
 *   gcc principal.c hash.c Fbloom.c -lm -o sistema
 *
 * EXECUÇÃO:
 *   ./sistema
 * =============================================================
 */

/* Necessário para clock_gettime() e struct timespec no padrão POSIX */
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hash.h"
#include "Fbloom.h"

/* =============================================================
 * Constantes
 * ============================================================= */

/* Capacidade inicial esperada do sistema.
 * Define o dimensionamento do Filtro de Bloom (m e k).
 * Alterar conforme o cenário de experimento (1k / 10k / 100k). */
#define CAPACIDADE_INICIAL 100000

/* Taxa de falso positivo alvo: 1% */
#define FP_RATE BLOOM_FP_PADRAO

/* Tamanho máximo de um nome de usuário */
#define MAX_USUARIO 50

/* =============================================================
 * Variáveis globais de estado e métricas
 * ============================================================= */

static BloomFilter *bloom = NULL; /* filtro de Bloom global */

/* Contadores de desempenho para RF03 */
static uint64_t total_elementos    = 0; /* elementos armazenados     */
static uint64_t total_consultas_g  = 0; /* consultas realizadas      */
static double   tempo_total_ns     = 0; /* tempo acumulado (ns)      */

/* =============================================================
 * Funções auxiliares de tempo
 * ============================================================= */

/* Retorna o tempo atual em nanosegundos (relógio monotônico) */
static double agora_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

/* =============================================================
 * Operações do sistema
 * ============================================================= */

/*
 * sistema_inserir — insere um usuário no Bloom e na Hash.
 * Sempre os dois devem ser atualizados juntos.
 */
static void sistema_inserir(const char *usuario)
{
    bloom_inserir(bloom, usuario);   /* marca bits no Bloom  */
    inserirHash(usuario);            /* armazena na Hash     */
    total_elementos++;
}

/*
 * sistema_consultar — consulta com o fluxo obrigatório do PDF (RF02):
 *   1. Bloom: se 0 → ausente com certeza, retorna imediatamente.
 *   2. Hash : confirma ou detecta falso positivo do Bloom.
 *
 * Retorna: 1 se encontrado, 0 se não encontrado.
 * Mede o tempo da operação e acumula em tempo_total_ns.
 */
static int sistema_consultar(const char *usuario)
{
    double t_inicio = agora_ns();
    total_consultas_g++;

    /* --- Etapa 1: consulta ao Filtro de Bloom --- */
    int bloom_resp = bloom_consultar(bloom, usuario);

    if (bloom_resp == 0) {
        /* Definitivamente não existe → evitamos a Hash */
        tempo_total_ns += agora_ns() - t_inicio;
        return 0;
    }

    /* --- Etapa 2: bloom disse "possivelmente existe" → confirmar na Hash --- */
    int hash_resp = buscarHash(usuario);

    if (!hash_resp) {
        /* Hash negou: era um falso positivo do Bloom */
        bloom_registrar_fp(bloom);
    }

    tempo_total_ns += agora_ns() - t_inicio;
    return hash_resp;
}

/*
 * sistema_carregar_arquivo — RF04: lê usuários de um arquivo texto,
 * um por linha, e insere cada um no sistema.
 */
static void sistema_carregar_arquivo(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) {
        printf("[ERRO] Nao foi possivel abrir o arquivo: %s\n", caminho);
        return;
    }

    char usuario[MAX_USUARIO];
    int  count = 0;

    while (fgets(usuario, sizeof(usuario), arq)) {
        /* Remove o '\n' do final */
        usuario[strcspn(usuario, "\n")] = '\0';

        if (strlen(usuario) > 0) {
            sistema_inserir(usuario);
            count++;
        }
    }

    fclose(arq);
    printf("[OK] %d usuarios carregados de '%s'\n", count, caminho);
}

/*
 * sistema_imprimir_estatisticas — RF03: exibe todas as métricas.
 */
static void sistema_imprimir_estatisticas(void)
{
    printf("\n========================================\n");
    printf("         ESTATISTICAS DO SISTEMA        \n");
    printf("========================================\n");
    printf("Elementos armazenados      : %lu\n", total_elementos);
    printf("Consultas realizadas       : %lu\n", total_consultas_g);

    /* Delega métricas do Bloom */
    printf("Consultas evitadas (Bloom) : %lu\n", bloom->consultas_evitadas);
    printf("Falsos positivos           : %lu\n", bloom->falsos_positivos);
    printf("Taxa FP real               : %.4f%%\n", bloom_taxa_fp_real(bloom));

    /* Tempo médio por consulta */
    if (total_consultas_g > 0) {
        double media_ns = tempo_total_ns / (double)total_consultas_g;
        printf("Tempo medio por consulta   : %.2f ns\n", media_ns);
    } else {
        printf("Tempo medio por consulta   : N/A\n");
    }

    /* Detalhes do Bloom */
    bloom_imprimir_info(bloom);
}

/* =============================================================
 * Experimentos (Parte 3 do PDF)
 * =============================================================
 *
 * experimento_sem_bloom — mede o tempo de busca de todos os
 * registros de um arquivo SEM usar o filtro de Bloom.
 * Serve como baseline de comparação.
 */
static void experimento_sem_bloom(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) {
        printf("[ERRO] Arquivo nao encontrado: %s\n", caminho);
        return;
    }

    char     usuario[MAX_USUARIO];
    uint64_t buscas = 0;
    double   t_inicio = agora_ns();

    while (fgets(usuario, sizeof(usuario), arq)) {
        usuario[strcspn(usuario, "\n")] = '\0';
        if (strlen(usuario) > 0) {
            buscarHash(usuario);   /* busca direta na Hash, sem Bloom */
            buscas++;
        }
    }

    double t_total = agora_ns() - t_inicio;
    fclose(arq);

    printf("\n--- Experimento SEM Bloom (%s) ---\n", caminho);
    printf("Registros buscados : %lu\n", buscas);
    printf("Tempo total        : %.2f ms\n", t_total / 1e6);
    if (buscas > 0)
        printf("Tempo medio        : %.2f ns/busca\n", t_total / (double)buscas);
}

/*
 * experimento_com_bloom — mede o tempo de busca dos mesmos
 * registros COM o filtro de Bloom ativo.
 */
static void experimento_com_bloom(const char *caminho)
{
    FILE *arq = fopen(caminho, "r");
    if (!arq) {
        printf("[ERRO] Arquivo nao encontrado: %s\n", caminho);
        return;
    }

    char     usuario[MAX_USUARIO];
    uint64_t buscas = 0;
    double   t_inicio = agora_ns();

    while (fgets(usuario, sizeof(usuario), arq)) {
        usuario[strcspn(usuario, "\n")] = '\0';
        if (strlen(usuario) > 0) {
            /* Usa o fluxo completo com Bloom */
            if (bloom_consultar(bloom, usuario)) {
                buscarHash(usuario);
            }
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

/* =============================================================
 * Menu interativo
 * ============================================================= */

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

/* =============================================================
 * main
 * ============================================================= */
int main(void)
{
    /* --- Inicialização --- */
    inicializarHash();

    bloom = bloom_criar(CAPACIDADE_INICIAL, FP_RATE);
    if (!bloom) {
        fprintf(stderr, "[ERRO FATAL] Nao foi possivel criar o Filtro de Bloom.\n");
        return EXIT_FAILURE;
    }

    printf("Sistema iniciado.\n");
    printf("Bloom: m=%lu bits | k=%u funcoes hash | FP alvo=%.2f%%\n",
           bloom->m, bloom->k, bloom->fp_alvo * 100.0);

    /* --- Menu principal --- */
    int  opcao;
    char usuario[MAX_USUARIO];
    char caminho[256];

    do {
        imprimir_menu();
        if (scanf("%d", &opcao) != 1) break;
        getchar();   /* consome o '\n' após o número */

        switch (opcao) {

        /* RF01 — Inserção */
        case 1:
            printf("Nome do usuario: ");
            if (fgets(usuario, sizeof(usuario), stdin)) {
                usuario[strcspn(usuario, "\n")] = '\0';
                sistema_inserir(usuario);
                printf("[OK] Usuario '%s' inserido.\n", usuario);
            }
            break;

        /* RF02 — Consulta */
        case 2:
            printf("Nome do usuario: ");
            if (fgets(usuario, sizeof(usuario), stdin)) {
                usuario[strcspn(usuario, "\n")] = '\0';
                int encontrado = sistema_consultar(usuario);
                if (encontrado)
                    printf("-> Usuario encontrado\n");
                else
                    printf("-> Usuario inexistente\n");
            }
            break;

        /* RF03 — Estatísticas */
        case 3:
            sistema_imprimir_estatisticas();
            break;

        /* RF04 — Inserção em lote */
        case 4:
            printf("Caminho do arquivo: ");
            if (fgets(caminho, sizeof(caminho), stdin)) {
                caminho[strcspn(caminho, "\n")] = '\0';
                sistema_carregar_arquivo(caminho);
            }
            break;

        /* Parte 3 — Experimentos */
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

    /* --- Liberação de memória --- */
    bloom_destruir(bloom);
    return EXIT_SUCCESS;
}