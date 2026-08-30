#ifndef TIPOS_H
#define TIPOS_H

// os 18 tipos de pokemon (mesmo esquema do jogo original)
typedef enum {
    TIPO_NORMAL, TIPO_FOGO, TIPO_AGUA, TIPO_GRAMA, TIPO_ELETRICO, TIPO_GELO,
    TIPO_LUTADOR, TIPO_VENENOSO, TIPO_TERRA, TIPO_VOADOR, TIPO_PSIQUICO,
    TIPO_INSETO, TIPO_PEDRA, TIPO_FANTASMA, TIPO_DRAGAO, TIPO_SOMBRIO,
    TIPO_ACO, TIPO_FADA,
    NUM_TIPOS
} TipoPokemon;

const char* Tipo_nome(TipoPokemon t);
TipoPokemon Tipo_from_string(const char* nome);

// multiplicador de dano: 2x super efetivo, 0.5x pouco efetivo, 0 imune, 1 neutro
double Tipo_multiplicador(TipoPokemon atacante, TipoPokemon defensor);

// mesma coisa mas pra pokemon com 2 tipos (multiplica os dois)
double Tipo_multiplicador_combinado(TipoPokemon atacante, TipoPokemon def1, int tem_tipo2, TipoPokemon def2);

#endif
