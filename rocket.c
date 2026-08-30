#include <stdio.h>
#include <stdlib.h>
#include "rocket.h"
#include "batalha.h"

EquipeRocket* Rocket_criar(const char* nome, int vertice_inicial) {
    EquipeRocket* r = (EquipeRocket*)malloc(sizeof(EquipeRocket));
    r->treinador = Treinador_criar(nome, vertice_inicial, 0);
    r->escondido = 0;
    r->tempo_escondido = 0;
    return r;
}

void Rocket_tentar_roubo(EquipeRocket* rocket, Treinador* alvo, Grafo* g, int vertice_ataque, int usar_tipos, int* tempo_gasto) {
    if (rocket->escondido) return;

    printf("=== A Equipe Rocket ataca %s! ===\n", alvo->nome);
    int rocket_venceu = Batalha_treinador_vs_treinador(rocket->treinador, alvo, usar_tipos, tempo_gasto);

    if (rocket_venceu) {
        if (alvo->num_pokemons > 1) {
            // rouba um pokemon aleatorio (nunca deixa o cara com 0)
            int indice = rand() % alvo->num_pokemons;
            Pokemon* roubado = alvo->pokemons[indice];
            for (int i = indice; i < alvo->num_pokemons - 1; i++) alvo->pokemons[i] = alvo->pokemons[i + 1];
            alvo->num_pokemons--;
            printf("  A Equipe Rocket roubou %s de %s!\n", roubado->nome, alvo->nome);
            Treinador_adicionar_pokemon(rocket->treinador, roubado);
        } else if (alvo->num_insignias_conquistadas > 0) {
            // sem pokemon sobrando, rouba uma insignia
            for (int i = 0; i < alvo->num_ginasios; i++) {
                if (alvo->insignias[i]) {
                    alvo->insignias[i] = 0;
                    alvo->num_insignias_conquistadas--;
                    printf("  A Equipe Rocket roubou uma insignia de %s!\n", alvo->nome);
                    break;
                }
            }
        }

        rocket->escondido = 1;
        rocket->tempo_escondido = 20 + rand() % 61;
    } else {
        // perdeu, manda ela pro vertice mais longe do ataque
        int destino_longe = Grafo_vertice_mais_distante(g, vertice_ataque);
        rocket->treinador->vertice_atual = destino_longe;
        printf("  A Equipe Rocket foi derrotada e enviada para longe (vertice %d).\n", destino_longe);
    }
}

void Rocket_atualizar_tempo(EquipeRocket* rocket, Grafo* g, int unidades_tempo) {
    if (!rocket->escondido) return;
    rocket->tempo_escondido -= unidades_tempo;
    if (rocket->tempo_escondido <= 0) {
        rocket->escondido = 0;
        rocket->treinador->vertice_atual = rand() % g->num_vertices;
        printf("  A Equipe Rocket reapareceu no vertice %d.\n", rocket->treinador->vertice_atual);
    }
}

void Rocket_liberar(EquipeRocket* rocket) {
    Treinador_liberar(rocket->treinador);
    free(rocket);
}
