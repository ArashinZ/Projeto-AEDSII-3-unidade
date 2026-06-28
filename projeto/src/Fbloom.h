#ifndef FBLOOM_H
#define FBLOOM_H
 
/*
 * Fbloom.h - Filtro de Bloom
 *
 * bloom_consultar() == 0 -> elemento CERTAMENTE ausente
 * bloom_consultar() == 1 -> pode existir (confirmar na hash)
 *
 * m = ceil(-(n * ln p) / (ln 2)^2)
 * k = round((m / n) * ln 2)
 */
 
#include <stdint.h>
#include <stdlib.h>
 
#define BLOOM_FP_PADRAO 0.01
 
typedef struct {
    uint8_t  *bits;
    uint64_t  m;                  /* bits do vetor */
    uint32_t  k;                  /* funcoes hash  */
    uint64_t  n_esperado;
    uint64_t  n_inseridos;
    double    fp_alvo;
    uint64_t  total_consultas;
    uint64_t  consultas_evitadas;
    uint64_t  falsos_positivos;
} BloomFilter;
 
/* Cria o filtro para n_esperado elementos com taxa de FP fp_rate */
BloomFilter *bloom_criar(uint64_t n_esperado, double fp_rate);
void         bloom_destruir(BloomFilter *bf);
 
/* Inserir chave (chamar junto com inserirHash) */
void         bloom_inserir(BloomFilter *bf, const char *chave);
 
/* 0 = ausente (nao ir a hash), 1 = possivel (ir a hash) */
int          bloom_consultar(BloomFilter *bf, const char *chave);
 
/* Chamar quando bloom retornou 1 mas hash nao encontrou */
void         bloom_registrar_fp(BloomFilter *bf);
 
void         bloom_imprimir_info(const BloomFilter *bf);
double       bloom_taxa_fp_real(const BloomFilter *bf);
uint64_t     bloom_memoria_bytes(const BloomFilter *bf);
uint64_t     hash_sdbm(const char *str); /* para comparacao experimental */
 
#endif /* FBLOOM_H */
 
