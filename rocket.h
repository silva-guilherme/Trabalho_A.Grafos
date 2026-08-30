#ifndef ROCKET_H
#define ROCKET_H
#include "treinador.h"
#include "grafo.h"

// a equipe rocket é basicamente um treinador (reaproveita a struct toda)
typedef struct EquipeRocket {
    Treinador* treinador;
    int escondido;
    int tempo_escondido;
} EquipeRocket;

EquipeRocket* Rocket_criar(const char* nome, int vertice_inicial);

// se ganhar, rouba pokemon (ou insignia) e some por um tempo. se perder, vai pro vertice mais longe
void Rocket_tentar_roubo(EquipeRocket* rocket, Treinador* alvo, Grafo* g, int vertice_ataque, int usar_tipos, int* tempo_gasto);

// chama isso a cada tanto de tempo pra ela reaparecer quando acabar o tempo escondida
void Rocket_atualizar_tempo(EquipeRocket* rocket, Grafo* g, int unidades_tempo);

void Rocket_liberar(EquipeRocket* rocket);

#endif
