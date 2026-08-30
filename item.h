#ifndef ITEM_H
#define ITEM_H
#include "treinador.h"

// item simples: erva que cura o time. da pra criar outros tipos aqui depois
typedef struct ItemErva {
    int vertice;
    int coletado;
} ItemErva;

void Item_usar(Treinador* t, ItemErva* item);

#endif
