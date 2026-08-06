#ifndef GRAFO_H
#define GRAFO_H


typedef enum {
    COMUM,
    LABORATORIO,
    PMC,
    GINASIO,
    ESTADIO
} TipoVertice;

typedef struct Aresta {
    int destino;
    int peso;              /* tempo de percurso */
    struct Aresta* prox;
    struct Aresta* ant;
} Aresta;

typedef struct Vertice {
    Aresta* inicio;
    TipoVertice tipo;
    char nome_ginasio[50];  /* usado somente se tipo == GINASIO */
    int indice_insignia;    /* usado somente se tipo == GINASIO: posicao no vetor de insignias do treinador */
} Vertice;

typedef struct Grafo {
    int num_vertices;
    int num_ginasios;
    Vertice* vertices;
    long soma_pesos;        /* soma de todos os pesos das arestas (usada para calcular o prazo de inscricao) */
} Grafo;

Grafo* Grafo_criar(int num_vertices);


void Grafo_adicionar_aresta(Grafo* g, int origem, int destino, int peso);
void Grafo_adicionar_aresta_bidirecional(Grafo* g, int a, int b, int peso);

void Grafo_definir_tipo(Grafo* g, int vertice, TipoVertice tipo);
void Grafo_definir_ginasio(Grafo* g, int vertice, const char* nome, int indice_insignia);

void Grafo_imprimir(Grafo* g);


int* Grafo_dijkstra(Grafo* g, int origem, int* anterior);

int* Grafo_reconstruir_caminho(int* anterior, int origem, int destino, int* tamanho);

int Grafo_vertice_mais_distante(Grafo* g, int origem);

void Grafo_liberar(Grafo* g);

#endif
