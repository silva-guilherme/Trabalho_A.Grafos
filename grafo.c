#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "grafo.h"

Grafo* Grafo_criar(int num_vertices) {
    Grafo* g = (Grafo*)malloc(sizeof(Grafo));
    g->num_vertices = num_vertices;
    g->num_ginasios = 0;
    g->soma_pesos = 0;
    g->vertices = (Vertice*)malloc(num_vertices * sizeof(Vertice));
    for (int i = 0; i < num_vertices; i++) {
        g->vertices[i].inicio = NULL;
        g->vertices[i].tipo = COMUM;
        g->vertices[i].nome_ginasio[0] = '\0';
        g->vertices[i].indice_insignia = -1;
    }
    return g;
}

void Grafo_adicionar_aresta(Grafo* g, int origem, int destino, int peso) {
    Aresta* nova = (Aresta*)malloc(sizeof(Aresta));
    nova->destino = destino;
    nova->peso = peso;
    nova->prox = g->vertices[origem].inicio;
    nova->ant = NULL;

    if (g->vertices[origem].inicio != NULL) {
        g->vertices[origem].inicio->ant = nova;
    }
    g->vertices[origem].inicio = nova;
    g->soma_pesos += peso;
}

void Grafo_adicionar_aresta_bidirecional(Grafo* g, int a, int b, int peso) {
    Grafo_adicionar_aresta(g, a, b, peso);
    Grafo_adicionar_aresta(g, b, a, peso);
}

void Grafo_definir_tipo(Grafo* g, int vertice, TipoVertice tipo) {
    g->vertices[vertice].tipo = tipo;
    if (tipo == GINASIO) g->num_ginasios++;
}

void Grafo_definir_ginasio(Grafo* g, int vertice, const char* nome, int indice_insignia) {
    g->vertices[vertice].tipo = GINASIO;
    strncpy(g->vertices[vertice].nome_ginasio, nome, 49);
    g->vertices[vertice].nome_ginasio[49] = '\0';
    g->vertices[vertice].indice_insignia = indice_insignia;
    g->num_ginasios++;
}

static const char* nome_tipo_vertice(TipoVertice t) {
    switch (t) {
        case LABORATORIO: return "Laboratorio do Prof. Carvalho";
        case PMC: return "Centro Medico Pokemon (PMC)";
        case GINASIO: return "Ginasio";
        case ESTADIO: return "Estadio da Liga";
        default: return "Comum";
    }
}

void Grafo_imprimir(Grafo* g) {
    for (int i = 0; i < g->num_vertices; i++) {
        printf("Vertice %d [%s%s%s]: ", i, nome_tipo_vertice(g->vertices[i].tipo),
               g->vertices[i].tipo == GINASIO ? " - " : "",
               g->vertices[i].tipo == GINASIO ? g->vertices[i].nome_ginasio : "");
        Aresta* atual = g->vertices[i].inicio;
        while (atual != NULL) {
            printf("<-> [Dest: %d, Tempo: %d] ", atual->destino, atual->peso);
            atual = atual->prox;
        }
        printf("\n");
    }
}


typedef struct {
    int* vertice;  
    int* posicao;  
    int* dist;      
    int tamanho;
} MinHeap;

static void heap_swap(MinHeap* h, int i, int j) {
    int vi = h->vertice[i], vj = h->vertice[j];
    h->vertice[i] = vj; h->vertice[j] = vi;
    h->posicao[vi] = j; h->posicao[vj] = i;
}

static void heap_desce(MinHeap* h, int i) {
    int menor = i;
    int e = 2 * i + 1, d = 2 * i + 2;
    if (e < h->tamanho && h->dist[h->vertice[e]] < h->dist[h->vertice[menor]]) menor = e;
    if (d < h->tamanho && h->dist[h->vertice[d]] < h->dist[h->vertice[menor]]) menor = d;
    if (menor != i) {
        heap_swap(h, i, menor);
        heap_desce(h, menor);
    }
}

static void heap_sobe(MinHeap* h, int i) {
    while (i > 0) {
        int pai = (i - 1) / 2;
        if (h->dist[h->vertice[pai]] <= h->dist[h->vertice[i]]) break;
        heap_swap(h, i, pai);
        i = pai;
    }
}

static int heap_vazio(MinHeap* h) { return h->tamanho == 0; }

static int heap_extrair_min(MinHeap* h) {
    int raiz = h->vertice[0];
    h->tamanho--;
    h->vertice[0] = h->vertice[h->tamanho];
    h->posicao[h->vertice[0]] = 0;
    heap_desce(h, 0);
    h->posicao[raiz] = -1;
    return raiz;
}

static void heap_diminuir_chave(MinHeap* h, int v) {
    heap_sobe(h, h->posicao[v]);
}

int* Grafo_dijkstra(Grafo* g, int origem, int* anterior) {
    int n = g->num_vertices;
    int* dist = (int*)malloc(n * sizeof(int));

    MinHeap h;
    h.vertice = (int*)malloc(n * sizeof(int));
    h.posicao = (int*)malloc(n * sizeof(int));
    h.dist = dist;
    h.tamanho = n;

    for (int i = 0; i < n; i++) {
        dist[i] = INT_MAX;
        h.vertice[i] = i;
        h.posicao[i] = i;
        if (anterior != NULL) anterior[i] = -1;
    }
    dist[origem] = 0;
    heap_sobe(&h, h.posicao[origem]);

    while (!heap_vazio(&h)) {
        int u = heap_extrair_min(&h);
        if (dist[u] == INT_MAX) continue; 

        Aresta* a = g->vertices[u].inicio;
        while (a != NULL) {
            int v = a->destino;
            if (h.posicao[v] != -1 && dist[u] + a->peso < dist[v]) {
                dist[v] = dist[u] + a->peso;
                if (anterior != NULL) anterior[v] = u;
                heap_diminuir_chave(&h, v);
            }
            a = a->prox;
        }
    }

    free(h.vertice);
    free(h.posicao);
    return dist;
}

int* Grafo_reconstruir_caminho(int* anterior, int origem, int destino, int* tamanho) {
    if (destino != origem && anterior[destino] == -1) {
        *tamanho = 0;
        return NULL;
    }
    int capacidade = 16, n = 0;
    int* pilha = (int*)malloc(capacidade * sizeof(int));
    int atual = destino;
    while (atual != -1) {
        if (n >= capacidade) { capacidade *= 2; pilha = (int*)realloc(pilha, capacidade * sizeof(int)); }
        pilha[n++] = atual;
        if (atual == origem) break;
        atual = anterior[atual];
    }
    
    int* caminho = (int*)malloc(n * sizeof(int));
    for (int i = 0; i < n; i++) caminho[i] = pilha[n - 1 - i];
    free(pilha);
    *tamanho = n;
    return caminho;
}

int Grafo_vertice_mais_distante(Grafo* g, int origem) {
    int* dist = Grafo_dijkstra(g, origem, NULL);
    int melhor = origem, melhor_dist = -1;
    for (int i = 0; i < g->num_vertices; i++) {
        if (dist[i] != INT_MAX && dist[i] > melhor_dist) {
            melhor_dist = dist[i];
            melhor = i;
        }
    }
    free(dist);
    return melhor;
}

void Grafo_liberar(Grafo* g) {
    for (int i = 0; i < g->num_vertices; i++) {
        Aresta* atual = g->vertices[i].inicio;
        while (atual != NULL) {
            Aresta* prox = atual->prox;
            free(atual);
            atual = prox;
        }
    }
    free(g->vertices);
    free(g);
}
