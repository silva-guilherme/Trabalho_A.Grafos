#ifndef POKEMON_H
#define POKEMON_H
#include "tipos.h"

#define MAX_FASES 3

// uma especie = a linha evolutiva toda (nomes das 3 fases + tipo)
// varios Pokemon podem apontar pra mesma especie
typedef struct EspeciePokemon {
    char nomes_fase[MAX_FASES][50];
    int num_fases_definidas;
    TipoPokemon tipos[2];
    int num_tipos;
} EspeciePokemon;

typedef enum {
    CONSCIENTE,
    INCONSCIENTE,
    MACHUCADO
} EstadoPokemon;

typedef struct Pokemon Pokemon;

struct Pokemon {
    char nome[50];
    EspeciePokemon* especie;
    int fase;

    int hp;
    int xp;
    int xp_na_ultima_evolucao; // pra saber quando bateu 1000 desde a ultima vez

    int ap_base, dp_base;
    int ap_bonus_batalha; // +1 permanente quando vence alguem com xp >= o seu
    int dp_bonus_batalha;

    // bonus temporario, só vale durante uma luta entre treinadores (vale o xp do treinador)
    // fica zerado o resto do tempo
    int bonus_treinador_ap;
    int bonus_treinador_dp;

    EstadoPokemon estado;
    int tempo_indisponivel; // contagem regressiva (inconsciente OU tratamento no pmc)

    int vertice_atual;
    int tem_treinador;

    void (*receber_dano)(Pokemon* this, int dano);
    void (*ganhar_xp)(Pokemon* this, int quantidade);
};

EspeciePokemon* Especie_criar(const char* nomes[], int num_fases, TipoPokemon t1, int tem_t2, TipoPokemon t2);

Pokemon* Pokemon_criar(EspeciePokemon* especie, int ap_inicial, int dp_inicial);

// pokemon que nasceu de ovo (basicamente igual ao normal, so separado pra deixar claro a origem)
Pokemon* Pokemon_criar_de_ovo(EspeciePokemon* especie, int ap_inicial, int dp_inicial);

int Pokemon_ap_total(const Pokemon* p);
int Pokemon_dp_total(const Pokemon* p);

void Pokemon_atualizar_estado(Pokemon* p);

// +1 hp a cada 10 unidades de tempo, ate 100 (nao funciona se tiver machucado)
void Pokemon_recuperar_hp_por_tempo(Pokemon* p, int unidades_tempo);

// erva: +10 hp, so em quem ta consciente
void Pokemon_usar_remedio(Pokemon* p);

void Pokemon_curar_no_pmc(Pokemon* p);

// vai descontando o tempo de tratamento; retorna 1 quando termina de curar
int Pokemon_tratar_no_pmc(Pokemon* p, int tempo_no_pmc);

void Pokemon_xp_por_distancia(Pokemon* p, int distancia_percorrida);

// evolui se já acumulou 1000 xp desde a ultima evolucao. retorna 1 se evoluiu
int Pokemon_tentar_evoluir(Pokemon* p);

void Pokemon_bonus_vitoria(Pokemon* p, int xp_oponente_antes_da_batalha);

void Pokemon_liberar(Pokemon* p);

#endif
