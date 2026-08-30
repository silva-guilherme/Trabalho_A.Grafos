#ifndef ITEM_H
#define ITEM_H
#include "treinador.h"

/* ==========================================================================
 * MODULO ITEM (ervas / remedios naturais) -- requisito adicional 4/5
 * Um item encontrado no mapa. Ao ser usado, aumenta em 10 os HP's de todos
 * os pokemons CONSCIENTES do treinador (ate o maximo de 100), conforme o
 * enunciado. Pokemons inconscientes/machucados nao sao afetados (nao
 * conseguem tomar o remedio).
 * ========================================================================== */

typedef struct ItemErva {
    int vertice;
    int coletado; /* 0 = ainda disponivel no mapa */
} ItemErva;

/* Aplica o efeito da erva a todo o time consciente do treinador e marca o
 * item como coletado. */
void Item_usar(Treinador* t, ItemErva* item);

#endif
