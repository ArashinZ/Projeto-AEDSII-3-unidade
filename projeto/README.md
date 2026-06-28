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

para compilar os testes: 

cd testes
gcc experimentos.c hash.c Fbloom.c -lm -o experimentos

para executar:

./experimentos

## Estrutura do Projeto

projeto/
├── src/
│   ├── hash.h 
│   ├── hash.c          
│   ├── Fbloom.h        
│   ├── Fbloom.c        
│   └── principal.c 
│   └── experimentos.c
│
├── data/
│   ├── usuarios_1k.txt 
│   ├── usuarios_10k.txt
│   └── usuarios_100k.txt
│
├── README.md
└── relatorio.pdf
