#ifndef BATALHA_H
#define BATALHA_H
#include "pokemon.h"
#include "treinador.h"

// duelo até um dos dois cair. quem chama decide quem ataca primeiro
Pokemon* Batalha_duelo_pokemon(Pokemon* primeiro, Pokemon* segundo, int usar_tipos);

// decide se aceita o desafio (precisa ter 3 conscientes, e chance menor contra alguem com xp bem maior)
int Batalha_desafiado_aceita(const Treinador* desafiante, const Treinador* desafiado);

// batalha 3x3 completa. se o desafiado nao tiver 3 conscientes, ganha de W.O.
int Batalha_treinador_vs_treinador(Treinador* desafiante, Treinador* desafiado, int usar_tipos, int* tempo_gasto);

// tenta capturar (pode desistir antes se o selvagem parecer forte demais)
int Batalha_capturar_selvagem(Treinador* t, Pokemon* selvagem, int usar_tipos, int* tempo_gasto);

#endif
