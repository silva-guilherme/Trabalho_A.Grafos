#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "batalha.h"

// quanto maior a diferenca de xp, maior a chance (max 90%). usei 1000 como escala
// porque é o mesmo tanto de xp que precisa pra evoluir
static double probabilidade_por_xp(int xp_a, int xp_b) {
    int diferenca = abs(xp_a - xp_b);
    double p = diferenca / 1000.0 * 0.9;
    if (p > 0.9) p = 0.9;
    return p;
}

static int sorteio(double probabilidade) {
    return ((double)rand() / RAND_MAX) < probabilidade;
}

static int executar_ataque(Pokemon* atacante, Pokemon* defensor, int usar_tipos) {
    // esquiva: proporcional a quao mais xp o defensor tem
    double prob_esquiva = probabilidade_por_xp(defensor->xp, atacante->xp);
    if (sorteio(prob_esquiva)) {
        printf("    %s se esquivou do ataque de %s!\n", defensor->nome, atacante->nome);
        return 0;
    }

    int dano = Pokemon_ap_total(atacante) - Pokemon_dp_total(defensor);
    if (dano <= 0) return 0;

    if (usar_tipos) {
        TipoPokemon t1 = defensor->especie->tipos[0];
        int tem_t2 = defensor->especie->num_tipos > 1;
        TipoPokemon t2 = tem_t2 ? defensor->especie->tipos[1] : t1;
        double mult = Tipo_multiplicador_combinado(atacante->especie->tipos[0], t1, tem_t2, t2);
        dano = (int)(dano * mult);
    }

    // critico: mesma logica da esquiva, mas do lado do atacante
    double prob_critico = probabilidade_por_xp(atacante->xp, defensor->xp);
    if (sorteio(prob_critico)) {
        dano *= 2;
        printf("    Golpe critico de %s!\n", atacante->nome);
    }

    if (dano < 0) dano = 0;
    defensor->receber_dano(defensor, dano);
    return dano;
}

// limite de seguranca: se nenhum lado consegue causar dano isso nunca terminaria sozinho
#define LIMITE_TURNOS 500

Pokemon* Batalha_duelo_pokemon(Pokemon* primeiro, Pokemon* segundo, int usar_tipos) {
    Pokemon* atacante = primeiro;
    Pokemon* defensor = segundo;
    int turnos = 0;

    while (primeiro->estado == CONSCIENTE && segundo->estado == CONSCIENTE && turnos < LIMITE_TURNOS) {
        executar_ataque(atacante, defensor, usar_tipos);
        turnos++;
        if (defensor->estado != CONSCIENTE) break;
        // troca quem ataca
        Pokemon* tmp = atacante; atacante = defensor; defensor = tmp;
    }

    Pokemon* vencedor;
    Pokemon* perdedor;
    if (primeiro->estado == CONSCIENTE && segundo->estado != CONSCIENTE) {
        vencedor = primeiro; perdedor = segundo;
    } else if (segundo->estado == CONSCIENTE && primeiro->estado != CONSCIENTE) {
        vencedor = segundo; perdedor = primeiro;
    } else {
        // bateu no limite de turnos, desempata por hp e depois por xp
        if (primeiro->hp != segundo->hp) {
            vencedor = primeiro->hp > segundo->hp ? primeiro : segundo;
        } else {
            vencedor = primeiro->xp >= segundo->xp ? primeiro : segundo;
        }
        perdedor = (vencedor == primeiro) ? segundo : primeiro;
    }

    // vitoria = +10 xp, derrota = +3 xp (vale pra qualquer duelo, inclusive captura)
    vencedor->ganhar_xp(vencedor, 10);
    perdedor->ganhar_xp(perdedor, 3);

    return vencedor;
}

int Batalha_desafiado_aceita(const Treinador* desafiante, const Treinador* desafiado) {
    if (Treinador_contar_conscientes(desafiado) < 3) return 0; // nem da pra aceitar

    double prob_aceitar = 0.85;
    if (desafiante->xp > desafiado->xp) {
        int diferenca = desafiante->xp - desafiado->xp;
        prob_aceitar -= (diferenca / 100.0) * 0.05;
        if (prob_aceitar < 0.3) prob_aceitar = 0.3;
    }
    return sorteio(prob_aceitar);
}

// zera o bonus temporario depois da luta, pra nao vazar pra outra batalha ou captura
static void limpar_bonus_treinador(Treinador* t) {
    for (int i = 0; i < t->num_pokemons; i++) {
        t->pokemons[i]->bonus_treinador_ap = 0;
        t->pokemons[i]->bonus_treinador_dp = 0;
    }
}

int Batalha_treinador_vs_treinador(Treinador* desafiante, Treinador* desafiado, int usar_tipos, int* tempo_gasto) {
    int idx_a[3], idx_b[3];
    if (!Treinador_escolher_3_para_batalha(desafiante, idx_a)) {
        printf("  %s nao tem 3 pokemons conscientes para batalhar.\n", desafiante->nome);
        return 0;
    }
    if (!Treinador_escolher_3_para_batalha(desafiado, idx_b)) {
        // desafiado ja ta sem condicao de lutar, entao o desafiante ganha direto
        printf("  %s nao tem 3 pokemons conscientes: %s vence por W.O.!\n", desafiado->nome, desafiante->nome);
        desafiante->xp += (desafiante->xp >= desafiado->xp) ? 3 : 1;
        return 1;
    }

    printf("--- Batalha: %s (desafiante) vs %s (desafiado) ---\n", desafiante->nome, desafiado->nome);

    int prox_a = 0, prox_b = 0;
    Pokemon* ativo_a = desafiante->pokemons[idx_a[prox_a++]];
    Pokemon* ativo_b = desafiado->pokemons[idx_b[prox_b++]];

    while (1) {
        // bonus temporario = xp do treinador (so vale entre treinadores diferentes)
        ativo_a->bonus_treinador_ap = desafiante->xp;
        ativo_a->bonus_treinador_dp = desafiante->xp;
        ativo_b->bonus_treinador_ap = desafiado->xp;
        ativo_b->bonus_treinador_dp = desafiado->xp;

        int xp_a_antes = ativo_a->xp, xp_b_antes = ativo_b->xp;

        // o desafiado ataca primeiro
        Pokemon* vencedor_duelo = Batalha_duelo_pokemon(ativo_b, ativo_a, usar_tipos);
        if (tempo_gasto != NULL) *tempo_gasto += 1;

        if (vencedor_duelo == ativo_b) {
            Pokemon_bonus_vitoria(ativo_b, xp_a_antes);
            if (prox_a < 3) {
                ativo_a = desafiante->pokemons[idx_a[prox_a++]];
            } else {
                // desafiante perdeu os 3
                desafiado->xp += (desafiado->xp >= desafiante->xp) ? 3 : 1;
                printf("--- %s venceu a batalha! ---\n", desafiado->nome);
                limpar_bonus_treinador(desafiante);
                limpar_bonus_treinador(desafiado);
                return 0;
            }
        } else {
            Pokemon_bonus_vitoria(ativo_a, xp_b_antes);
            if (prox_b < 3) {
                ativo_b = desafiado->pokemons[idx_b[prox_b++]];
            } else {
                desafiante->xp += (desafiante->xp >= desafiado->xp) ? 3 : 1;
                printf("--- %s venceu a batalha! ---\n", desafiante->nome);
                limpar_bonus_treinador(desafiante);
                limpar_bonus_treinador(desafiado);
                return 1;
            }
        }
    }
}

// chance de desistir cresce se o selvagem parece bem mais forte que meu pokemon
static int treinador_desiste_da_captura(Pokemon* meu, Pokemon* selvagem) {
    int vantagem_selvagem = Pokemon_dp_total(selvagem) - Pokemon_ap_total(meu);
    if (vantagem_selvagem <= 0) return 0;
    double prob_desistir = vantagem_selvagem / 100.0;
    if (prob_desistir > 0.6) prob_desistir = 0.6;
    return sorteio(prob_desistir);
}

int Batalha_capturar_selvagem(Treinador* t, Pokemon* selvagem, int usar_tipos, int* tempo_gasto) {
    Pokemon* meu = NULL;
    for (int i = 0; i < t->num_pokemons; i++) {
        if (t->pokemons[i]->estado == CONSCIENTE) { meu = t->pokemons[i]; break; }
    }
    if (meu == NULL) {
        printf("  %s nao tem pokemon consciente para tentar a captura.\n", t->nome);
        return 0;
    }

    if (treinador_desiste_da_captura(meu, selvagem)) {
        printf("  %s avaliou %s selvagem e desistiu da captura, deixando-o fugir.\n", t->nome, selvagem->nome);
        return 0;
    }

    printf("--- %s tenta capturar %s selvagem ---\n", t->nome, selvagem->nome);
    int xp_selvagem_antes = selvagem->xp;
    Pokemon* vencedor = Batalha_duelo_pokemon(meu, selvagem, usar_tipos);
    if (tempo_gasto != NULL) *tempo_gasto += 1;

    if (vencedor == meu) {
        printf("  %s foi capturado por %s!\n", selvagem->nome, t->nome);
        Pokemon_bonus_vitoria(meu, xp_selvagem_antes);
        t->xp += 3;
        meu->ganhar_xp(meu, 3);
        selvagem->ganhar_xp(selvagem, 3);
        Treinador_adicionar_pokemon(t, selvagem);
        return 1;
    }
    printf("  A captura falhou; %s fugiu.\n", selvagem->nome);
    return 0;
}
