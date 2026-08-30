#ifndef CONFIG_H
#define CONFIG_H
#include "grafo.h"
#include "pokemon.h"



typedef struct ConfigJogo {
    Grafo* grafo;

    EspeciePokemon** especies;
    int num_especies;

    int num_pokemons_selvagens;
    int num_treinadores;
    int num_itens;

    int vertice_laboratorio;
    int vertice_pmc;
    int vertice_estadio;

    long prazo_inscricao; 
} ConfigJogo;


ConfigJogo* Config_ler_arquivo(const char* caminho_arquivo);

void Config_liberar(ConfigJogo* cfg);

#endif
