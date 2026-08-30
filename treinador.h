#ifndef TREINADOR_H
#define TREINADOR_H
#include "pokemon.h"
#include "grafo.h"

#define MAX_POKEMONS_ATIVOS 6
#define NUM_POKEBOLAS_INICIAL 7

typedef struct Treinador {
    char nome[50];
    int xp;
    int vertice_atual;

    Pokemon* pokemons[MAX_POKEMONS_ATIVOS];
    int num_pokemons;

    int tem_ovo; // só pode ter 1 por vez
    Pokemon* ovo;
    int ovo_progresso;

    int pokebolas_disponiveis;

    int* insignias; // um bool por ginasio (pra saber quais ja venceu)
    int num_ginasios;
    int num_insignias_conquistadas;

    int registrado_na_liga;
} Treinador;

Treinador* Treinador_criar(const char* nome, int vertice_inicial, int num_ginasios);

// se o time ja tiver 6, manda o excedente pro professor (retorna 0)
int Treinador_adicionar_pokemon(Treinador* t, Pokemon* p);

int Treinador_encontrar_ovo(Treinador* t, EspeciePokemon* especie_desconhecida_placeholder, int ap, int dp);

// eclode em 100 de distancia e ja entra pro time se tiver vaga
void Treinador_avancar_ovo(Treinador* t, int distancia);

int Treinador_contar_conscientes(const Treinador* t);

// pega os 3 primeiros conscientes pra batalha
int Treinador_escolher_3_para_batalha(const Treinador* t, int indices_saida[3]);

// anda 1 vertice na direção do destino (usa dijkstra). retorna o tempo gasto, 0 se ja chegou
int Treinador_dar_passo(Treinador* t, Grafo* g, int destino);

void Treinador_ganhar_insignia(Treinador* t, int indice_insignia);

int Treinador_pode_se_inscrever(const Treinador* t);

void Treinador_liberar(Treinador* t);

#endif
