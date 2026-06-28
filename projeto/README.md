# Sistema de Consulta Eficiente com Tabela Hash e Filtro de Bloom
<<<<<<< HEAD

## Descrição

Sistema de verificação de cadastro de usuários que integra duas estruturas de dados complementares:

- **Tabela Hash** com encadeamento externo — armazenamento e recuperação exata
- **Filtro de Bloom** — estrutura probabilística que acelera consultas de existência
=======
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
>>>>>>> 0d17d8c1d7b9c3c5a64a9bc034643dec8109a487

O Filtro de Bloom atua como "porteiro" antes da Tabela Hash: se ele afirma que um usuário **definitivamente não existe**, a Hash não é consultada. Se diz "possivelmente existe", a Hash confirma.

---

<<<<<<< HEAD
=======
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

>>>>>>> 0d17d8c1d7b9c3c5a64a9bc034643dec8109a487
## Estrutura do Projeto

```
projeto/
├── src/
│   ├── hash.c          — Implementação da Tabela Hash
│   ├── hash.h          — Interface da Tabela Hash
│   ├── Fbloom.c        — Implementação do Filtro de Bloom
│   ├── Fbloom.h        — Interface do Filtro de Bloom
│   ├── principal.c     — Menu interativo principal
│   └── experimentos.c  — Programa de experimentos automatizados (Parte 3)
├── data/
<<<<<<< HEAD
│   ├── usuarios_1k.txt     — 1.000 usuários para inserção
│   ├── usuarios_10k.txt    — 10.000 usuários para inserção
│   ├── usuarios_100k.txt   — 100.000 usuários para inserção
│   ├── busca_1000.txt      — 1.000 buscas (50% presentes + 50% ausentes)
│   ├── busca_10000.txt     — 10.000 buscas mistas
│   └── busca_100000.txt    — 100.000 buscas mistas
└── README.md
```

---

## Compilação

### Sistema interativo (menu)
```bash
cd src
gcc principal.c hash.c Fbloom.c -lm -o sistema
```

### Experimentos automatizados (Parte 3)
```bash
cd src
gcc experimentos.c hash.c Fbloom.c -lm -o experimentos
```

> **Dependências:** apenas a biblioteca padrão C + `-lm` (math). Não há dependências externas.

---

## Execução

### Sistema interativo
```bash
./sistema
```

Menu disponível:
```
  1. INSERIR usuario
  2. CONSULTAR usuario
  3. ESTATISTICAS
  4. CARREGAR arquivo (lote)
  5. EXPERIMENTO sem Bloom vs com Bloom
  0. SAIR
```

### Experimentos automatizados
```bash
cd src
./experimentos
```
Roda os três cenários (1k / 10k / 100k) automaticamente e exibe a tabela de resultados.

---

## Formato de Entrada

### Inserção manual (opção 1)
```
Nome do usuario: joao123
```

### Carregamento em lote (opção 4)
Arquivo `.txt` com um usuário por linha:
```
joao123
maria98
pedro45
```

### Formato dos arquivos gerados (Parte 3)
Padrão: **8 letras minúsculas + 3 dígitos**
```
islaifda122
djskalsa297
fjkldsaf881
```

---

## Exemplos de Execução

### Inserção e consulta
```
Opcao: 1
Nome do usuario: alice99
[OK] Usuario 'alice99' inserido.

Opcao: 2
Nome do usuario: alice99
-> Usuario encontrado

Opcao: 2
Nome do usuario: bob999
-> Usuario inexistente
```

### Estatísticas (opção 3)
```
========================================
         ESTATISTICAS DO SISTEMA
========================================
Elementos armazenados      : 1
Consultas realizadas       : 2
Consultas evitadas (Bloom) : 1
Falsos positivos           : 0
Taxa FP real               : 0.0000%
Tempo medio por consulta   : 1842.50 ns
```

---

## Decisões de Projeto

### Tabela Hash
- **Tamanho:** 100.017 posições (número primo)
- **Colisão:** encadeamento externo (listas encadeadas)
- **Fator de carga esperado:** ~1,0 para 100k elementos → desempenho O(1) médio
- **Função hash:** djb2 (D.J. Bernstein) — excelente distribuição para strings

### Filtro de Bloom
- **Taxa de falso positivo alvo:** 1%
- **Dimensionamento automático:** m e k calculados pelas fórmulas de Bloom (1970)
  - `m = ceil(-(n × ln p) / (ln 2)²)`
  - `k = round((m/n) × ln 2)`
- **Resultado:** k = 7 funções hash para FP = 1%
- **Técnica:** duplo hashing (Kirsch & Mitzenmacher, 2006) com djb2 + FNV-1a

---

## Integrantes

| Integrante | Responsabilidade |
|---|---|
| Integrante 1 | hash.c / hash.h — Tabela Hash |
| Integrante 2 | Fbloom.c / Fbloom.h — Filtro de Bloom |
| Integrante 3 | principal.c / experimentos.c + relatório |
=======
│   ├── usuarios_1k.txt 
│   ├── usuarios_10k.txt
│   └── usuarios_100k.txt
│
├── README.md
└── relatorio.pdf
>>>>>>> 0d17d8c1d7b9c3c5a64a9bc034643dec8109a487
