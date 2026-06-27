#ifndef HASH_H
#define HASH_H

#define HASH_SIZE 100017 //numero primo, ajuda bastante pra evitar colisao

typedef struct No{
    char usuario[50];
    struct No *prox;
} No;

void inicializarHash();
void inserirHash(const char *usuario);
int buscarHash(const char *usuario);
void liberarhash();

#endif