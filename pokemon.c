#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pokemon.h"

static void Pokemon_receber_dano_impl(Pokemon* this, int dano) {
    this->hp -= dano;
    if (this->hp < 1) this->hp = 1; // nao pode ficar em 0, so no pmc mesmo
    Pokemon_atualizar_estado(this);
}

static void Pokemon_ganhar_xp_impl(Pokemon* this, int quantidade) {
    this->xp += quantidade;
    Pokemon_tentar_evoluir(this);
}

EspeciePokemon* Especie_criar(const char* nomes[], int num_fases, TipoPokemon t1, int tem_t2, TipoPokemon t2) {
    EspeciePokemon* e = (EspeciePokemon*)malloc(sizeof(EspeciePokemon));
    e->num_fases_definidas = num_fases;
    for (int i = 0; i < num_fases && i < MAX_FASES; i++) {
        strncpy(e->nomes_fase[i], nomes[i], 49);
        e->nomes_fase[i][49] = '\0';
    }
    e->tipos[0] = t1;
    e->num_tipos = 1;
    if (tem_t2) { e->tipos[1] = t2; e->num_tipos = 2; }
    return e;
}

static Pokemon* Pokemon_alocar_base(EspeciePokemon* especie, int ap_inicial, int dp_inicial) {
    Pokemon* p = (Pokemon*)malloc(sizeof(Pokemon));
    p->especie = especie;
    p->fase = 0;
    strncpy(p->nome, especie->nomes_fase[0], 49);
    p->nome[49] = '\0';

    p->hp = 100;
    p->xp = 0;
    p->xp_na_ultima_evolucao = 0;

    p->ap_base = ap_inicial;
    p->dp_base = dp_inicial;
    p->ap_bonus_batalha = 0;
    p->dp_bonus_batalha = 0;
    p->bonus_treinador_ap = 0;
    p->bonus_treinador_dp = 0;

    p->estado = CONSCIENTE;
    p->tempo_indisponivel = 0;
    p->vertice_atual = 0;
    p->tem_treinador = 0;

    p->receber_dano = Pokemon_receber_dano_impl;
    p->ganhar_xp = Pokemon_ganhar_xp_impl;
    return p;
}

Pokemon* Pokemon_criar(EspeciePokemon* especie, int ap_inicial, int dp_inicial) {
    return Pokemon_alocar_base(especie, ap_inicial, dp_inicial);
}

Pokemon* Pokemon_criar_de_ovo(EspeciePokemon* especie, int ap_inicial, int dp_inicial) {
    // é a mesma coisa, só separei a funcao pra deixar claro que veio de ovo
    return Pokemon_alocar_base(especie, ap_inicial, dp_inicial);
}

int Pokemon_ap_total(const Pokemon* p) {
    return p->ap_base + (p->xp * 10) / 100 + p->ap_bonus_batalha + p->bonus_treinador_ap;
}

int Pokemon_dp_total(const Pokemon* p) {
    return p->dp_base + (p->xp * 10) / 100 + p->dp_bonus_batalha + p->bonus_treinador_dp;
}

void Pokemon_atualizar_estado(Pokemon* p) {
    if (p->hp < 5) {
        p->estado = MACHUCADO;
        if (p->tempo_indisponivel <= 0) {
            p->tempo_indisponivel = 10 + rand() % 41; // tempo de tratamento no pmc
        }
    } else if (p->hp < 20) {
        p->estado = INCONSCIENTE;
        if (p->tempo_indisponivel <= 0) {
            p->tempo_indisponivel = 10 + rand() % 41;
        }
    } else {
        p->estado = CONSCIENTE;
        p->tempo_indisponivel = 0;
    }
}

void Pokemon_recuperar_hp_por_tempo(Pokemon* p, int unidades_tempo) {
    if (p->estado == MACHUCADO) return; // esse so cura no pmc

    int ganho = unidades_tempo / 10;
    if (ganho > 0) {
        p->hp += ganho;
        if (p->hp > 100) p->hp = 100;
    }
    if (p->tempo_indisponivel > 0) {
        p->tempo_indisponivel -= unidades_tempo;
        if (p->tempo_indisponivel < 0) p->tempo_indisponivel = 0;
    }
    if (p->tempo_indisponivel <= 0) Pokemon_atualizar_estado(p);
}

void Pokemon_usar_remedio(Pokemon* p) {
    if (p->estado != CONSCIENTE) return;
    p->hp += 10;
    if (p->hp > 100) p->hp = 100;
}

void Pokemon_curar_no_pmc(Pokemon* p) {
    p->hp = 100;
    p->tempo_indisponivel = 0;
    p->estado = CONSCIENTE;
}

int Pokemon_tratar_no_pmc(Pokemon* p, int tempo_no_pmc) {
    if (p->estado != MACHUCADO) return 0;
    p->tempo_indisponivel -= tempo_no_pmc;
    if (p->tempo_indisponivel <= 0) {
        Pokemon_curar_no_pmc(p);
        return 1;
    }
    return 0;
}

void Pokemon_xp_por_distancia(Pokemon* p, int distancia_percorrida) {
    int ganho = distancia_percorrida / 100;
    if (ganho > 0) p->ganhar_xp(p, ganho);
}

int Pokemon_tentar_evoluir(Pokemon* p) {
    if (p->xp - p->xp_na_ultima_evolucao < 1000) return 0;
    if (p->fase + 1 >= p->especie->num_fases_definidas) return 0; // ja ta na ultima fase

    p->fase++;
    strncpy(p->nome, p->especie->nomes_fase[p->fase], 49);
    p->nome[49] = '\0';
    p->ap_base = (int)(p->ap_base * 1.3);
    p->dp_base = (int)(p->dp_base * 1.3);
    p->xp_na_ultima_evolucao = p->xp;
    return 1;
}

void Pokemon_bonus_vitoria(Pokemon* p, int xp_oponente_antes_da_batalha) {
    if (xp_oponente_antes_da_batalha >= p->xp) {
        p->ap_bonus_batalha += 1;
        p->dp_bonus_batalha += 1;
    }
}

void Pokemon_liberar(Pokemon* p) {
    free(p);
}
