# Sistema de Consulta Eficiente com Tabela Hash e Filtro de Bloom
## Visão Geral
O nosso sistema envolve a verificação de cadastro de usuários utilizando linguagem C, usando tabela hash e filtro de bloom.

## Funcionalidades
- ✅ Cadastra um novo usuário por identificador único
- ✅ Consulta com dois estágios com fluxo obrigatorio na consulta do filtro de bloom
- ✅ Exibe métricas em tempo real de:
    - Total de elementos armazenados
    - Total de consultas realizadas
    - Consultas evitadas pelo bloom
    - Número e taxa de falsos positivos 
    - Tempo médio de consulta
- ✅ Inseção em lote carregando um arquivo .txt com varios usuários de uma vez

## Tecnologias Utilizadas
### Linguagens
- C
### Bibliotecas
- stdio.h
- stdlib.h
- string.h
- time.h
- math.h
## Pré-requisitos
Para rodar o projeto, instale:

- MinGW: https://sourceforge.net/projects/mingw/
- Expansão C/C++ no VsCode
## Instalação e Execução
Clone o repositório:

git clone https://github.com/ArashinZ/Projeto-AEDSII-3-unidade.git

Entre na pasta do projeto:

cd projeto
cd source

Compile o código:

gcc principal.c hash.c fbloom.c -lm -o sistema

Para executar o projeto:

./sistema

## Estrutura do Projeto
```
projeto/
├── data/
│   ├── usuarios_1000k.txt 
│   ├── usuarios_10000k.txt
│   ├── usuarios_100000k.txt
│   ├── usuarios_100k.txt 
│   ├── usuarios_10k.txt
│   └── usuarios_1k.txt
├── src/       
│   ├── Fbloom.c        
│   ├── Fbloom.h
│   ├── experimentos.c
│   ├── hash.c 
│   ├── hash.h  
│   └── principal.c
│   └── README.md
