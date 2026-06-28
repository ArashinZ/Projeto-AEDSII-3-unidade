/*
 * Fbloom.c - Implementacao do Filtro de Bloom
 *
 * Hashes base: djb2 (h1) e FNV-1a (h2), ambos 64 bits.
 * Posicionamento: duplo hashing — pos_i = (h1 + i*h2) % m
 * Formulas: m = ceil(-(n*ln p)/(ln 2)^2),  k = round((m/n)*ln 2)
 */
 
#include "Fbloom.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
 
/* djb2, 64 bits */
static uint64_t hash_djb2(const char *str)
{
    uint64_t h = 5381;
    while (*str)
        h = ((h << 5) + h) + (unsigned char)*str++;
    return h;
}
 
/* FNV-1a, 64 bits */
static uint64_t hash_fnv1a(const char *str)
{
    uint64_t h = 14695981039346656037ULL;
    while (*str) {
        h ^= (unsigned char)*str++;
        h *= 1099511628211ULL;
    }
    return h;
}
 
/* sdbm, 64 bits — exposto para comparacao experimental */
uint64_t hash_sdbm(const char *str)
{
    uint64_t h = 0;
    while (*str)
        h = (unsigned char)*str++ + (h << 6) + (h << 16) - h;
    return h;
}
 
/* bit i: byte bits[i/8], posicao i%8 */
#define BIT_SET(arr, i)  ((arr)[(i)/8] |=  (uint8_t)(1u << ((i)%8)))
#define BIT_GET(arr, i)  ((arr)[(i)/8] &   (uint8_t)(1u << ((i)%8)))
 
BloomFilter *bloom_criar(uint64_t n_esperado, double fp_rate)
{
    if (n_esperado == 0 || fp_rate <= 0.0 || fp_rate >= 1.0) {
        fprintf(stderr, "[Bloom] parametros invalidos: n=%lu, fp=%.4f\n",
                n_esperado, fp_rate);
        return NULL;
    }
 
    BloomFilter *bf = (BloomFilter *)malloc(sizeof(BloomFilter));
    if (!bf) return NULL;
 
    const double ln2 = log(2.0);
 
    /* m = ceil(-(n * ln p) / (ln 2)^2) */
    uint64_t m = (uint64_t)ceil(-(double)n_esperado * log(fp_rate) / (ln2 * ln2));
    if (m < 8) m = 8;
 
    /* k = round((m / n) * ln 2) */
    uint32_t k = (uint32_t)round((double)m / (double)n_esperado * ln2);
    if (k < 1) k = 1;
 
    uint64_t n_bytes = (m + 7) / 8;
    uint8_t *bits = (uint8_t *)calloc(n_bytes, sizeof(uint8_t));
    if (!bits) { free(bf); return NULL; }
 
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
 
void bloom_destruir(BloomFilter *bf)
{
    if (!bf) return;
    free(bf->bits);
    free(bf);
}
 
/* pos_i = (h1 + i*h2) % m; h2 forcado impar para cobrir todo o vetor */
static void bloom_calcular_posicoes(const BloomFilter *bf,
                                    const char *chave,
                                    uint64_t *pos)
{
    uint64_t h1 = hash_djb2(chave);
    uint64_t h2 = hash_fnv1a(chave);
    if (h2 % 2 == 0) h2++;
 
    for (uint32_t i = 0; i < bf->k; i++)
        pos[i] = (h1 + (uint64_t)i * h2) % bf->m;
}
 
void bloom_inserir(BloomFilter *bf, const char *chave)
{
    if (!bf || !chave) return;
    uint64_t pos[bf->k];
    bloom_calcular_posicoes(bf, chave, pos);
    for (uint32_t i = 0; i < bf->k; i++)
        BIT_SET(bf->bits, pos[i]);
    bf->n_inseridos++;
}
 
/* Retorna 0 = ausente com certeza, 1 = possivelmente presente */
int bloom_consultar(BloomFilter *bf, const char *chave)
{
    if (!bf || !chave) return 0;
    bf->total_consultas++;
 
    uint64_t pos[bf->k];
    bloom_calcular_posicoes(bf, chave, pos);
 
    for (uint32_t i = 0; i < bf->k; i++) {
        if (!BIT_GET(bf->bits, pos[i])) {
            bf->consultas_evitadas++;
            return 0;
        }
    }
    return 1;
}
 
void bloom_registrar_fp(BloomFilter *bf)
{
    if (bf) bf->falsos_positivos++;
}
 
double bloom_taxa_fp_real(const BloomFilter *bf)
{
    if (!bf) return 0.0;
    uint64_t chegaram = bf->total_consultas - bf->consultas_evitadas;
    if (chegaram == 0) return 0.0;
    return (double)bf->falsos_positivos / (double)chegaram * 100.0;
}
 
uint64_t bloom_memoria_bytes(const BloomFilter *bf)
{
    if (!bf) return 0;
    return (bf->m + 7) / 8 + (uint64_t)sizeof(BloomFilter);
}
 
void bloom_imprimir_info(const BloomFilter *bf)
{
    if (!bf) return;
    printf("\n========================================\n");
    printf("         FILTRO DE BLOOM — INFO         \n");
    printf("========================================\n");
    printf("Elementos esperados (n)    : %lu\n",   bf->n_esperado);
    printf("Tamanho do vetor (m)       : %lu bits  (%.2f KB)\n",
           bf->m, (double)((bf->m + 7) / 8) / 1024.0);
    printf("Funcoes hash (k)           : %u\n",    bf->k);
    printf("Taxa FP alvo               : %.4f%%\n", bf->fp_alvo * 100.0);
    printf("----------------------------------------\n");
    printf("Elementos inseridos        : %lu\n",   bf->n_inseridos);
    printf("----------------------------------------\n");
    printf("Total de consultas         : %lu\n",   bf->total_consultas);
    printf("Consultas evitadas (Bloom) : %lu\n",   bf->consultas_evitadas);
    printf("Consultas na Hash          : %lu\n",
           bf->total_consultas - bf->consultas_evitadas);
    printf("Falsos positivos           : %lu\n",   bf->falsos_positivos);
    printf("Taxa FP real               : %.4f%%\n", bloom_taxa_fp_real(bf));
    printf("----------------------------------------\n");
    printf("Memoria total              : %lu bytes  (%.2f KB)\n",
           bloom_memoria_bytes(bf),
           (double)bloom_memoria_bytes(bf) / 1024.0);
    printf("========================================\n\n");
}
 
