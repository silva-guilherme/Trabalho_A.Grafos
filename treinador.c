#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "treinador.h"

Treinador* Treinador_criar(const char* nome, int vertice_inicial, int num_ginasios) {
    Treinador* t = (Treinador*)malloc(sizeof(Treinador));
    strncpy(t->nome, nome, 49);
    t->nome[49] = '\0';
    t->xp = 0;
    t->vertice_atual = vertice_inicial;
    t->num_pokemons = 0;
    t->tem_ovo = 0;
    t->ovo = NULL;
    t->ovo_progresso = 0;
    t->pokebolas_disponiveis = 1; /* 1 pokebola livre para novas capturas; as outras 6 sao "slots" do time */
    t->num_ginasios = num_ginasios;
    t->insignias = (int*)calloc(num_ginasios > 0 ? num_ginasios : 1, sizeof(int));
    t->num_insignias_conquistadas = 0;
    t->registrado_na_liga = 0;
    return t;
}

int Treinador_adicionar_pokemon(Treinador* t, Pokemon* p) {
    if (t->num_pokemons >= MAX_POKEMONS_ATIVOS) {
        /* Time cheio: pokemon excedente vai para estudos do Prof. Carvalho */
        printf("  [Prof. Carvalho] %s ja tem 6 pokemons; %s foi enviado para estudos.\n", t->nome, p->nome);
        Pokemon_liberar(p);
        return 0;
    }
    p->tem_treinador = 1;
    t->pokemons[t->num_pokemons++] = p;
    return 1;
}

int Treinador_encontrar_ovo(Treinador* t, EspeciePokemon* especie_oculta, int ap, int dp) {
    /* So pode carregar um ovo nao-eclodido por vez, e o total (ativos + ovo)
     * nao pode passar de 7. */
    if (t->tem_ovo) return 0;
    if (t->num_pokemons >= MAX_POKEMONS_ATIVOS + 1) return 0;

    t->ovo = Pokemon_criar_de_ovo(especie_oculta, ap, dp);
    t->ovo_progresso = 0;
    t->tem_ovo = 1;
    return 1;
}

void Treinador_avancar_ovo(Treinador* t, int distancia) {
    if (!t->tem_ovo) return;
    t->ovo_progresso += distancia;
    if (t->ovo_progresso >= 100) {
        printf("  [Encubadora] O ovo de %s eclodiu! Um %s nasceu.\n", t->nome, t->ovo->nome);
        Pokemon* nascido = t->ovo;
        t->tem_ovo = 0;
        t->ovo = NULL;
        t->ovo_progresso = 0;
        Treinador_adicionar_pokemon(t, nascido);
    }
}

int Treinador_contar_conscientes(const Treinador* t) {
    int c = 0;
    for (int i = 0; i < t->num_pokemons; i++) {
        if (t->pokemons[i]->estado == CONSCIENTE) c++;
    }
    return c;
}

int Treinador_escolher_3_para_batalha(const Treinador* t, int indices_saida[3]) {
    int achados = 0;
    for (int i = 0; i < t->num_pokemons && achados < 3; i++) {
        if (t->pokemons[i]->estado == CONSCIENTE) {
            indices_saida[achados++] = i;
        }
    }
    return achados == 3;
}

int Treinador_viajar_ate(Treinador* t, Grafo* g, int destino) {
    if (t->vertice_atual == destino) return 0;

    int* anterior = (int*)malloc(g->num_vertices * sizeof(int));
    int* dist = Grafo_dijkstra(g, t->vertice_atual, anterior);
    int tamanho;
    int* caminho = Grafo_reconstruir_caminho(anterior, t->vertice_atual, destino, &tamanho);

    int tempo_total = 0;
    if (caminho != NULL) {
        /* percorre o caminho vertice a vertice (um por vez), acumulando o
         * tempo (peso) de cada aresta percorrida */
        for (int i = 0; i + 1 < tamanho; i++) {
            int u = caminho[i], v = caminho[i + 1];
            Aresta* a = g->vertices[u].inicio;
            int peso_aresta = 0;
            while (a != NULL) { if (a->destino == v) { peso_aresta = a->peso; break; } a = a->prox; }
            tempo_total += peso_aresta;
        }
        t->vertice_atual = destino;
        free(caminho);
    }
    free(dist);
    free(anterior);

    /* efeitos do tempo percorrido sobre o time */
    for (int i = 0; i < t->num_pokemons; i++) {
        Pokemon_recuperar_hp_por_tempo(t->pokemons[i], tempo_total);
        Pokemon_xp_por_distancia(t->pokemons[i], tempo_total);
    }
    Treinador_avancar_ovo(t, tempo_total);

    return tempo_total;
}

int Treinador_dar_passo(Treinador* t, Grafo* g, int destino) {
    if (t->vertice_atual == destino) return 0;

    int* anterior = (int*)malloc(g->num_vertices * sizeof(int));
    int* dist = Grafo_dijkstra(g, t->vertice_atual, anterior);
    int tamanho;
    int* caminho = Grafo_reconstruir_caminho(anterior, t->vertice_atual, destino, &tamanho);

    int tempo_passo = 0;
    if (caminho != NULL && tamanho > 1) {
        int atual = caminho[0], proximo = caminho[1];
        Aresta* a = g->vertices[atual].inicio;
        while (a != NULL) { if (a->destino == proximo) { tempo_passo = a->peso; break; } a = a->prox; }
        t->vertice_atual = proximo;
    }
    if (caminho != NULL) free(caminho);
    free(dist);
    free(anterior);

    /* efeitos do tempo percorrido nesta unica aresta */
    for (int i = 0; i < t->num_pokemons; i++) {
        Pokemon_recuperar_hp_por_tempo(t->pokemons[i], tempo_passo);
        Pokemon_xp_por_distancia(t->pokemons[i], tempo_passo);
    }
    Treinador_avancar_ovo(t, tempo_passo);

    return tempo_passo;
}

void Treinador_ganhar_insignia(Treinador* t, int indice_insignia) {
    if (indice_insignia < 0 || indice_insignia >= t->num_ginasios) return;
    if (!t->insignias[indice_insignia]) {
        t->insignias[indice_insignia] = 1;
        t->num_insignias_conquistadas++;
        printf("  [Liga] %s conquistou uma nova insignia! Total: %d\n", t->nome, t->num_insignias_conquistadas);
    }
}

int Treinador_pode_se_inscrever(const Treinador* t) {
    int necessarias = t->num_ginasios > 8 ? 8 : t->num_ginasios;
    return t->num_insignias_conquistadas >= necessarias;
}

void Treinador_liberar(Treinador* t) {
    for (int i = 0; i < t->num_pokemons; i++) Pokemon_liberar(t->pokemons[i]);
    if (t->ovo != NULL) Pokemon_liberar(t->ovo);
    free(t->insignias);
    free(t);
}
