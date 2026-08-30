#ifndef TREINADOR_H
#define TREINADOR_H
#include "pokemon.h"
#include "grafo.h"

/* ==========================================================================
 * MODULO TREINADOR
 * ========================================================================== */

#define MAX_POKEMONS_ATIVOS 6
#define NUM_POKEBOLAS_INICIAL 7 /* 6 para treino/batalha + 1 para novas capturas */

typedef struct Treinador {
    char nome[50];
    int xp;
    int vertice_atual;

    Pokemon* pokemons[MAX_POKEMONS_ATIVOS];
    int num_pokemons;

    int tem_ovo;
    Pokemon* ovo;            /* alocado com xp=0, mas so entra para o time quando eclode */
    int ovo_progresso;       /* distancia acumulada rumo aos 100 necessarios para eclodir */

    int pokebolas_disponiveis; /* comeca em NUM_POKEBOLAS_INICIAL - 6 (so a de captura conta aqui) */

    int* insignias;          /* vetor de booleans (0/1), tamanho = num_ginasios do grafo */
    int num_ginasios;
    int num_insignias_conquistadas;

    int registrado_na_liga;
} Treinador;

Treinador* Treinador_criar(const char* nome, int vertice_inicial, int num_ginasios);

/* Tenta adicionar um pokemon ao time. Se ja houver 6 ativos, o pokemon
 * excedente e enviado ao Prof. Carvalho (retorna 0); caso contrario entra
 * para o time (retorna 1). */
int Treinador_adicionar_pokemon(Treinador* t, Pokemon* p);

/* Encontra um ovo (so pode ter 1 nao-eclodido por vez, e total <= 7 contando ativos). */
int Treinador_encontrar_ovo(Treinador* t, EspeciePokemon* especie_desconhecida_placeholder, int ap, int dp);

/* Avanca o progresso do ovo com base na distancia percorrida; eclode ao
 * atingir 100 unidades e entra automaticamente para o time se houver vaga. */
void Treinador_avancar_ovo(Treinador* t, int distancia);

int Treinador_contar_conscientes(const Treinador* t);

/* Preenche 'indices_saida' (tamanho >= 3) com os indices de 3 pokemons
 * conscientes escolhidos para batalha. Retorna 1 se havia >= 3 conscientes. */
int Treinador_escolher_3_para_batalha(const Treinador* t, int indices_saida[3]);

/* Move o treinador (e seu time) do vertice atual ate 'destino' pelo
 * caminho minimo (Dijkstra), de uma vez so. Ao longo do percurso: os
 * pokemons recuperam HP com o tempo, ganham XP por distancia percorrida e
 * o ovo (se houver) avanca a incubacao. Retorna o tempo total gasto. */
int Treinador_viajar_ate(Treinador* t, Grafo* g, int destino);

/* Move o treinador UM UNICO VERTICE na direcao de 'destino' (o proximo
 * vertice do caminho minimo), como pede o enunciado ("cada pokemon e
 * treinador move-se um vertice por vez"). Aplica os efeitos de tempo (HP,
 * xp por distancia, incubacao) referentes apenas a essa aresta. Retorna o
 * tempo gasto nesse passo, ou 0 se ja estiver no destino. Chamar
 * repetidamente ate o retorno ser 0 percorre o caminho inteiro, permitindo
 * checar encontros (pokemons selvagens, itens, outros treinadores) a cada
 * parada intermediaria, nao so no destino final. */
int Treinador_dar_passo(Treinador* t, Grafo* g, int destino);

/* Concede a insignia do ginasio (indice) ao treinador, se ainda nao a possuir. */
void Treinador_ganhar_insignia(Treinador* t, int indice_insignia);

int Treinador_pode_se_inscrever(const Treinador* t);

void Treinador_liberar(Treinador* t);

#endif
