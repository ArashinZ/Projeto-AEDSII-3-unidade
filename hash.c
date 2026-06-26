#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

static No *tabelaHash[HASH_SIZE];

unsigned int hashBase(const char *str){
    unsigned int hash = 5381;

    while(*str){
        hash = ((hash << 5) + hash) + *str;
        str++;
    }

return hash;
}

unsigned int hashTabela(const char *str){
    return hashBase(str) % HASH_SIZE;
}

void inicializarHash(){
    for(int i = 0; i < HASH_SIZE; i++){
        tabelaHash[i] = NULL;
    }
}

void inserirHash(const char *usuario){
    unsigned int indice = hashTabela(usuario);

    No *novo = malloc(sizeof(No));
    strcpy(novo->usuario, usuario);
    novo->prox = tabelaHash[indice];
    tabelaHash[indice] = novo;
}

int buscarHash(const char *usuario){
    unsigned int indice = hashTabela(usuario);
    No *aux = tabelaHash[indice];

    while(aux){
        if(strcmp(aux->usuario, usuario) == 0){
            return 1;
        }

        aux = aux->prox;
    }

    return 0;
}