#include <stdio.h>
#include "item.h"

void Item_usar(Treinador* t, ItemErva* item) {
    if (item->coletado) return;
    item->coletado = 1;

    int quantos = 0;
    for (int i = 0; i < t->num_pokemons; i++) {
        if (t->pokemons[i]->estado == CONSCIENTE) {
            Pokemon_usar_remedio(t->pokemons[i]);
            quantos++;
        }
    }
    printf("  [Erva] %s encontrou uma erva especial e curou +10 HP de %d pokemon(s) consciente(s).\n", t->nome, quantos);
}
