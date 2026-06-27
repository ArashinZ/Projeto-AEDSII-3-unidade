/*
 * =============================================================
 * Fbloom.c — Implementação do Filtro de Bloom
 * =============================================================
 *
 * FUNÇÕES HASH BASE:
 *   h1 → djb2 64 bits   (D.J. Bernstein, 1991)
 *   h2 → FNV-1a 64 bits (Fowler / Noll / Vo)
 *   h3 → sdbm 64 bits   (utilitário sdbm do Unix)
 *
 *   Nota: o hash.c usa djb2 com 32 bits (unsigned int).
 *   Aqui usamos 64 bits para maior entropia e menor correlação.
 *
 * TÉCNICA DE POSICIONAMENTO (Kirsch & Mitzenmacher, 2006):
 *   Apenas dois hashes base são computados (h1 e h2).
 *   As k posições são geradas por duplo hashing:
 *     pos_i = ( h1(x) + i * h2(x) ) mod m,  i = 0, 1, ..., k-1
 *   Prova-se que isto tem o mesmo comportamento assintótico que
 *   k funções hash completamente independentes.
 *
 * FÓRMULAS DE DIMENSIONAMENTO:
 *   m = ceil( -(n * ln p) / (ln 2)^2 )
 *   k = round( (m / n) * ln 2 )
 *
 * COMPILAÇÃO:
 *   gcc -c Fbloom.c -lm -o Fbloom.o
 * =============================================================
 */

#include "Fbloom.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* =============================================================
 * Funções hash base — implementações manuais (sem bibliotecas)
 * ============================================================= */

/*
 * hash_djb2 — Daniel J. Bernstein (djb2), variante 64 bits.
 *
 * A constante inicial 5381 e o multiplicador implícito 33
 * (via "hash << 5 + hash") foram escolhidos empiricamente
 * por Bernstein por produzirem boa dispersão em strings.
 *
 * Diferença do hash.c: aqui usamos uint64_t em vez de
 * unsigned int, gerando um valor de 64 bits. Isso reduz
 * colisões e a correlação com a tabela hash.
 */
static uint64_t hash_djb2(const char *str)
{
    uint64_t h = 5381;
    while (*str)
        h = ((h << 5) + h) + (unsigned char)*str++;   /* h * 33 + c */
    return h;
}

/*
 * hash_fnv1a — FNV-1a 64 bits (Fowler / Noll / Vo).
 *
 * Aplica XOR antes da multiplicação ("XOR-then-multiply"),
 * ao contrário do FNV-1. Isso melhora o efeito avalanche
 * para strings com diferenças no início.
 *
 * As constantes (offset basis e primo FNV) são definidas
 * na especificação oficial do algoritmo.
 */
static uint64_t hash_fnv1a(const char *str)
{
    uint64_t h = 14695981039346656037ULL;   /* FNV offset basis 64 bits */
    while (*str) {
        h ^= (unsigned char)*str++;
        h *= 1099511628211ULL;              /* FNV primo 64 bits        */
    }
    return h;
}

/*
 * hash_sdbm — originalmente usada no banco de dados sdbm do Unix.
 *
 * Fórmula: h = c + (h << 6) + (h << 16) - h
 * Equivale a: h = c + 65599 * h
 * O multiplicador 65599 é primo e produz boa distribuição.
 */
/*
 * Exposta sem 'static' para uso comparativo experimental (Parte 3 do PDF):
 * pode substituir hash_fnv1a() em bloom_calcular_posicoes() para
 * comparar o impacto da função hash na taxa de falsos positivos.
 */
uint64_t hash_sdbm(const char *str)
{
    uint64_t h = 0;
    while (*str)
        h = (unsigned char)*str++ + (h << 6) + (h << 16) - h;
    return h;
}

/* =============================================================
 * Macros para manipulação direta de bits no vetor uint8_t.
 *
 * O bit de índice i está:
 *   - no byte  : bits[i / 8]
 *   - na posição: i % 8  (bit menos significativo = índice 0)
 *
 * BIT_SET → liga o bit i
 * BIT_GET → lê o bit i  (retorna 0 ou não-zero)
 * ============================================================= */
#define BIT_SET(arr, i)  ((arr)[(i) / 8] |=  (uint8_t)(1u << ((i) % 8)))
#define BIT_GET(arr, i)  ((arr)[(i) / 8] &   (uint8_t)(1u << ((i) % 8)))

/* =============================================================
 * bloom_criar
 *
 * Aplica as fórmulas de Bloom para calcular m (bits) e k (hashes),
 * aloca e inicializa o vetor de bits com zero (calloc).
 * ============================================================= */
BloomFilter *bloom_criar(uint64_t n_esperado, double fp_rate)
{
    /* Validação de parâmetros */
    if (n_esperado == 0 || fp_rate <= 0.0 || fp_rate >= 1.0) {
        fprintf(stderr,
                "[Bloom] ERRO: parametros invalidos. "
                "n_esperado=%lu, fp_rate=%.4f\n",
                n_esperado, fp_rate);
        return NULL;
    }

    BloomFilter *bf = (BloomFilter *)malloc(sizeof(BloomFilter));
    if (!bf) {
        fprintf(stderr, "[Bloom] ERRO: falha ao alocar a estrutura BloomFilter\n");
        return NULL;
    }

    /* -----------------------------------------------------------
     * Cálculo de m — tamanho do vetor de bits
     *
     *   m = ceil( -(n * ln p) / (ln 2)^2 )
     *
     * Intuição: quanto menor a taxa FP desejada (p) ou maior n,
     * maior deve ser o vetor para diluir as colisões entre bits.
     * ----------------------------------------------------------- */
    const double ln2    = log(2.0);          /* ln(2) ≈ 0.693147 */
    const double ln2_sq = ln2 * ln2;         /* (ln 2)^2          */

    uint64_t m = (uint64_t)ceil(
        -(double)n_esperado * log(fp_rate) / ln2_sq
    );
    if (m < 8) m = 8;   /* mínimo de 1 byte para evitar vetor nulo */

    /* -----------------------------------------------------------
     * Cálculo de k — número de funções hash
     *
     *   k = round( (m / n) * ln 2 )
     *
     * Intuição: k ótimo minimiza a taxa de falsos positivos para
     * um dado m e n. Poucas funções → mais bits 0 → menos FP.
     * Mas muitas funções → mais bits marcados → mais FP.
     * ----------------------------------------------------------- */
    uint32_t k = (uint32_t)round(
        (double)m / (double)n_esperado * ln2
    );
    if (k < 1) k = 1;   /* mínimo de 1 função hash */

    /* -----------------------------------------------------------
     * Alocação do vetor de bits
     * n_bytes = ceil(m / 8) — arredonda para cima para cobrir m bits
     * calloc inicializa tudo com 0 (todos os bits desligados)
     * ----------------------------------------------------------- */
    uint64_t n_bytes = (m + 7) / 8;
    uint8_t *bits = (uint8_t *)calloc(n_bytes, sizeof(uint8_t));
    if (!bits) {
        fprintf(stderr,
                "[Bloom] ERRO: falha ao alocar vetor de bits "
                "(%lu bytes)\n", n_bytes);
        free(bf);
        return NULL;
    }

    /* Preenche a estrutura */
    bf->bits               = bits;
    bf->m                  = m;
    bf->k                  = k;
    bf->n_esperado         = n_esperado;
    bf->n_inseridos        = 0;
    bf->fp_alvo            = fp_rate;
    bf->total_consultas    = 0;
    bf->consultas_evitadas = 0;
    bf->falsos_positivos   = 0;

    return bf;
}

/* =============================================================
 * bloom_destruir
 * ============================================================= */
void bloom_destruir(BloomFilter *bf)
{
    if (!bf) return;
    free(bf->bits);   /* libera o vetor de bits */
    free(bf);         /* libera a estrutura      */
}

/* =============================================================
 * bloom_calcular_posicoes  (uso interno)
 *
 * Calcula as k posições no vetor usando duplo hashing:
 *   pos_i = ( h1(x) + i * h2(x) ) mod m
 *
 * h1 → djb2   (boa distribuição geral)
 * h2 → fnv1a  (boa dispersão, baixa correlação com djb2)
 *
 * h2 é forçado a ser ímpar para garantir que o passo do
 * duplo hashing coprime com qualquer m (cobertura total).
 *
 * Parâmetros:
 *   bf    → filtro (fornece m e k)
 *   chave → string a ser mapeada
 *   pos   → array de saída com k posições (deve ter tamanho k)
 * ============================================================= */
static void bloom_calcular_posicoes(const BloomFilter *bf,
                                    const char *chave,
                                    uint64_t *pos)
{
    uint64_t h1 = hash_djb2(chave);
    uint64_t h2 = hash_fnv1a(chave);

    /* Garante que h2 seja ímpar (coprime com qualquer m) */
    if (h2 % 2 == 0) h2 += 1;

    /* Gera as k posições por duplo hashing */
    for (uint32_t i = 0; i < bf->k; i++) {
        pos[i] = (h1 + (uint64_t)i * h2) % bf->m;
    }
}

/* =============================================================
 * bloom_inserir
 *
 * Marca os k bits correspondentes à chave no vetor.
 * Deve ser chamado sempre que inserirHash() for chamado,
 * para manter os dois filtros sincronizados.
 * ============================================================= */
void bloom_inserir(BloomFilter *bf, const char *chave)
{
    if (!bf || !chave) return;

    uint64_t pos[bf->k];   /* array com as k posições */
    bloom_calcular_posicoes(bf, chave, pos);

    /* Liga cada um dos k bits */
    for (uint32_t i = 0; i < bf->k; i++) {
        BIT_SET(bf->bits, pos[i]);
    }

    bf->n_inseridos++;
}

/* =============================================================
 * bloom_consultar
 *
 * Verifica se TODOS os k bits da chave estão marcados (1).
 *
 * Se qualquer bit estiver em 0, o elemento DEFINITIVAMENTE
 * não foi inserido (sem falsos negativos).
 *
 * Se todos estiverem em 1, o elemento POSSIVELMENTE existe
 * (pode ser falso positivo — bits ligados por outras chaves).
 *
 * Retorna:
 *   0 → definitivamente NÃO existe  → evitar consulta à Hash
 *   1 → possivelmente existe        → confirmar na Hash
 * ============================================================= */
int bloom_consultar(BloomFilter *bf, const char *chave)
{
    if (!bf || !chave) return 0;

    bf->total_consultas++;

    uint64_t pos[bf->k];
    bloom_calcular_posicoes(bf, chave, pos);

    for (uint32_t i = 0; i < bf->k; i++) {
        if (!BIT_GET(bf->bits, pos[i])) {
            /*
             * Bit em 0: elemento definitivamente ausente.
             * Consulta à Hash EVITADA (economia garantida).
             */
            bf->consultas_evitadas++;
            return 0;
        }
    }

    /*
     * Todos os k bits estavam em 1: possivelmente existe.
     * O chamador deve confirmar na Tabela Hash.
     */
    return 1;
}

/* =============================================================
 * bloom_registrar_fp
 *
 * Registra um falso positivo.
 * Deve ser chamado pelo principal.c quando:
 *   bloom_consultar() retornou 1  (Bloom disse "existe")
 *   mas buscarHash()  retornou 0  (Hash confirmou "não existe")
 * ============================================================= */
void bloom_registrar_fp(BloomFilter *bf)
{
    if (!bf) return;
    bf->falsos_positivos++;
}

/* =============================================================
 * bloom_taxa_fp_real
 *
 * Calcula a taxa REAL de falsos positivos observada:
 *   taxa = falsos_positivos / consultas_que_chegaram_à_hash * 100
 *
 * "Consultas que chegaram à hash" = total - evitadas,
 * pois só essas tinham potencial de gerar falso positivo.
 *
 * Retorna: porcentagem (ex: 0.95 significa 0,95%)
 * ============================================================= */
double bloom_taxa_fp_real(const BloomFilter *bf)
{
    if (!bf) return 0.0;

    uint64_t chegaram_hash = bf->total_consultas - bf->consultas_evitadas;
    if (chegaram_hash == 0) return 0.0;

    return (double)bf->falsos_positivos / (double)chegaram_hash * 100.0;
}

/* =============================================================
 * bloom_memoria_bytes
 *
 * Retorna a memória total alocada pelo filtro:
 *   vetor de bits  +  struct BloomFilter
 * ============================================================= */
uint64_t bloom_memoria_bytes(const BloomFilter *bf)
{
    if (!bf) return 0;
    uint64_t bytes_bits   = (bf->m + 7) / 8;
    uint64_t bytes_struct = (uint64_t)sizeof(BloomFilter);
    return bytes_bits + bytes_struct;
}

/* =============================================================
 * bloom_imprimir_info
 *
 * Exibe o relatório completo do filtro (RF03):
 *   capacidade, m, k, inserções, consultas, falsos positivos, memória.
 * ============================================================= */
void bloom_imprimir_info(const BloomFilter *bf)
{
    if (!bf) return;

    printf("\n========================================\n");
    printf("         FILTRO DE BLOOM — INFO         \n");
    printf("========================================\n");

    /* Parâmetros de dimensionamento */
    printf("Elementos esperados (n)    : %lu\n",   bf->n_esperado);
    printf("Tamanho do vetor (m)       : %lu bits  (%.2f KB)\n",
           bf->m, (double)((bf->m + 7) / 8) / 1024.0);
    printf("Funcoes hash (k)           : %u\n",    bf->k);
    printf("Taxa FP alvo               : %.4f%%\n", bf->fp_alvo * 100.0);

    /* Estado atual */
    printf("----------------------------------------\n");
    printf("Elementos inseridos        : %lu\n",   bf->n_inseridos);

    /* Métricas de consulta (RF03) */
    printf("----------------------------------------\n");
    printf("Total de consultas         : %lu\n",   bf->total_consultas);
    printf("Consultas evitadas (Bloom) : %lu\n",   bf->consultas_evitadas);
    printf("Consultas na Hash          : %lu\n",
           bf->total_consultas - bf->consultas_evitadas);
    printf("Falsos positivos           : %lu\n",   bf->falsos_positivos);
    printf("Taxa FP real               : %.4f%%\n", bloom_taxa_fp_real(bf));

    /* Memória */
    printf("----------------------------------------\n");
    printf("Memoria total              : %lu bytes  (%.2f KB)\n",
           bloom_memoria_bytes(bf),
           (double)bloom_memoria_bytes(bf) / 1024.0);
    printf("========================================\n\n");
}