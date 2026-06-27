#ifndef FBLOOM_H
#define FBLOOM_H

/*
 * =============================================================
 * Fbloom.h — Filtro de Bloom (cabeçalho)
 * =============================================================
 *
 * Estrutura probabilística para consulta rápida de existência.
 *
 * GARANTIAS:
 *   - SEM falsos negativos: bloom_consultar() == 0 significa que
 *     o elemento CERTAMENTE não está na coleção.
 *   - COM falsos positivos: bloom_consultar() == 1 significa que
 *     o elemento PODE estar (confirmar na Tabela Hash).
 *
 * FÓRMULAS DE DIMENSIONAMENTO (Bloom, 1970):
 *
 *   m = ceil( -(n * ln p) / (ln 2)^2 )    [bits do vetor]
 *   k = round( (m / n) * ln 2 )            [funções hash]
 *
 *   onde:
 *     n = número esperado de elementos
 *     p = taxa de falso positivo desejada  (ex: 0.01 = 1%)
 *
 * TÉCNICA DE HASH:
 *   Duplo hashing (Kirsch & Mitzenmacher, 2006):
 *     pos_i = ( h1(x) + i * h2(x) ) mod m
 *   Simula k funções independentes com apenas dois hashes base.
 *
 * INTEGRAÇÃO COM hash.c:
 *   - Antes de buscarHash(), chamar bloom_consultar().
 *   - Se retornar 0: não buscar na hash (consulta evitada).
 *   - Se retornar 1: buscar na hash. Se hash não encontrar,
 *     chamar bloom_registrar_fp() para contabilizar o falso positivo.
 * =============================================================
 */

#include <stdint.h>
#include <stdlib.h>

/* Taxa de falso positivo padrão: 1% */
#define BLOOM_FP_PADRAO 0.01

/*
 * Estrutura principal do Filtro de Bloom.
 * Mantém o vetor de bits, os parâmetros de dimensionamento
 * e as métricas de desempenho para o relatório (RF03).
 */
typedef struct {
    uint8_t  *bits;               /* vetor de bits (1 bit por posição)  */
    uint64_t  m;                  /* tamanho total do vetor em bits      */
    uint32_t  k;                  /* número de funções hash              */
    uint64_t  n_esperado;         /* capacidade planejada no momento da criação */
    uint64_t  n_inseridos;        /* elementos efetivamente inseridos    */
    double    fp_alvo;            /* taxa de falso positivo desejada     */

    /* --- métricas de desempenho (RF03) --- */
    uint64_t  total_consultas;    /* total de chamadas a bloom_consultar */
    uint64_t  consultas_evitadas; /* Bloom retornou 0 (hash não consultada) */
    uint64_t  falsos_positivos;   /* Bloom retornou 1, mas hash negou    */
} BloomFilter;

/* =============================================================
 * Ciclo de vida
 * ============================================================= */

/*
 * bloom_criar — aloca e dimensiona o filtro.
 *
 * Parâmetros:
 *   n_esperado : número máximo de elementos que serão inseridos
 *   fp_rate    : taxa de falso positivo desejada (0 < fp_rate < 1)
 *
 * Retorna: ponteiro para BloomFilter, ou NULL em caso de erro.
 */
BloomFilter *bloom_criar(uint64_t n_esperado, double fp_rate);

/*
 * bloom_destruir — libera toda a memória do filtro.
 */
void bloom_destruir(BloomFilter *bf);

/* =============================================================
 * Operações
 * ============================================================= */

/*
 * bloom_inserir — marca as k posições da chave no vetor de bits.
 * Deve ser chamado sempre que inserirHash() for chamado.
 */
void bloom_inserir(BloomFilter *bf, const char *chave);

/*
 * bloom_consultar — verifica se os k bits da chave estão marcados.
 *
 * Retorna:
 *   0 → definitivamente NÃO existe → NÃO consultar a Hash
 *   1 → possivelmente existe       → consultar a Hash
 */
int bloom_consultar(BloomFilter *bf, const char *chave);

/*
 * bloom_registrar_fp — registra um falso positivo.
 * Chamar quando bloom_consultar() retornou 1, mas buscarHash()
 * confirmou que o elemento NÃO estava na coleção.
 */
void bloom_registrar_fp(BloomFilter *bf);

/* =============================================================
 * Utilitários e estatísticas (RF03)
 * ============================================================= */

/*
 * bloom_imprimir_info — exibe o relatório completo do filtro:
 *   capacidade, bits, funções hash, consultas, falsos positivos, etc.
 */
void bloom_imprimir_info(const BloomFilter *bf);

/*
 * bloom_taxa_fp_real — calcula a taxa real de falsos positivos.
 * Retorna porcentagem: (falsos_positivos / consultas_que_chegaram_à_hash) * 100
 */
double bloom_taxa_fp_real(const BloomFilter *bf);

/*
 * bloom_memoria_bytes — retorna o total de memória usada pelo filtro (em bytes).
 */
uint64_t bloom_memoria_bytes(const BloomFilter *bf);

/*
 * hash_sdbm — função hash sdbm exposta para fins de comparação experimental.
 * Pode ser usada na Parte 3 do PDF para comparar diferentes funções hash.
 * Substituir hash_fnv1a() em bloom_calcular_posicoes() e medir o impacto.
 */
uint64_t hash_sdbm(const char *str);

#endif /* FBLOOM_H */