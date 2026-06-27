#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

// Tabela hash global (encadeamento externo)
// Cada posição do vetor aponta para uma lista encadeada

static No *tabelaHash[HASH_SIZE];

unsigned int hashBase(const char *str){
    unsigned int hash = 5381; 

    while(*str){
         // hash * 33 + caractere atual <- formato da hash
        hash = ((hash << 5) + hash) + *str;
        str++;
    }

return hash;
}

// Função que adapta o hash ao tamanho da tabela
// Garante que o índice fique dentro do vetor

unsigned int hashTabela(const char *str){
    return hashBase(str) % HASH_SIZE;
}

// Inicializa a tabela hash
// Coloca NULL em todas as posições

void inicializarHash(){
    for(int i = 0; i < HASH_SIZE; i++){
        tabelaHash[i] = NULL;
    }
}

// Inserção na tabela hash (encadeamento externo)
// Em caso de colisão, adiciona no início da lista

void inserirHash(const char *usuario){
    unsigned int indice = hashTabela(usuario);
    No *novo = malloc(sizeof(No));
    if(novo == NULL){
        printf("Erro ao alocar memoria.\n");      //verifica se teve falha de alocaçao
        return;
    }
    strcpy(novo->usuario, usuario);
    novo->prox = tabelaHash[indice];   //copia o nome do usuario para o no e o insere na lista encadeada
    tabelaHash[indice] = novo;
}

// Busca na tabela hash
// Percorre a lista encadeada da posição calculada

int buscarHash(const char *usuario){
    unsigned int indice = hashTabela(usuario); //calcula o indice onde o usuario deve estar
    No *aux = tabelaHash[indice]; //ponteiro do usuario

    while(aux){
        if(strcmp(aux->usuario, usuario) == 0){
            return 1; //achou
        }

        aux = aux->prox;
    }
    return 0; //nao achou
}

// Libera toda a memória alocada pela tabela hash
// Evita vazamento de memória

void liberarHash(){
    for(int i = 0; i < HASH_SIZE; i++){
        No *atual = tabelaHash[i];

        while(atual != NULL){
            No *proximo = atual->prox;
            free(atual);
            atual = proximo;
        }

        tabelaHash[i] = NULL;
    }
}