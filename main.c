#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "grafo.h"
#include "tipos.h"
#include "pokemon.h"
#include "treinador.h"
#include "batalha.h"
#include "rocket.h"
#include "config.h"
#include "item.h"

#define USAR_VANTAGENS_DE_TIPO 1   /* item extra: 1 = ativa vantagens/desvantagens de tipo nas batalhas */
#define MAX_RODADAS 60
#define NUM_AGENTES_ROCKET 2       /* item extra: quantos agentes da Equipe Rocket rondam a regiao */
#define PROB_RECUSAR_STARTERS 0.15 /* chance de um treinador recusar os 3 iniciais e receber 1 aleatorio */
#define PROB_ACHAR_OVO_NO_CAMINHO 0.04 /* chance, por passo, de encontrar um ovo se ainda nao tiver um */

typedef struct {
    Pokemon* pokemon;
    int capturado; /* 0 = ainda selvagem/livre no mapa */
} PokemonSelvagem;

/* Agrupa tudo que descreve o estado do mundo simulado, para nao precisar
 * passar uma dezena de parametros soltos entre as funcoes. */
typedef struct {
    ConfigJogo* cfg;
    Treinador** treinadores;
    int num_treinadores;
    Treinador** lideres;
    int* vertices_ginasio;
    int num_ginasios;
    PokemonSelvagem* selvagens;
    ItemErva* itens;
    EquipeRocket** agentes_rocket;
    long tempo_decorrido;
} Mundo;

static int ap_aleatorio(void) { return 8 + rand() % 13; }  /* 8..20 */
static int dp_aleatorio(void) { return 8 + rand() % 13; }
static int sorteio_simples(double probabilidade) { return ((double)rand() / RAND_MAX) < probabilidade; }

/* Encontra todos os vertices do grafo marcados como GINASIO. */
static int* localizar_ginasios(Grafo* g, int* quantidade) {
    int* lista = (int*)malloc(g->num_vertices * sizeof(int));
    int n = 0;
    for (int i = 0; i < g->num_vertices; i++) {
        if (g->vertices[i].tipo == GINASIO) lista[n++] = i;
    }
    *quantidade = n;
    return lista;
}

/* Escolhe um vertice aleatorio que nao seja o laboratorio nem o PMC (usado
 * para posicionar pokemons selvagens e itens, que nao podem ficar nesses
 * dois locais). */
static int vertice_aleatorio_valido(ConfigJogo* cfg) {
    int v;
    do { v = rand() % cfg->grafo->num_vertices; }
    while (v == cfg->vertice_laboratorio || v == cfg->vertice_pmc);
    return v;
}

/* Monta um time fixo de 3 pokemons para um lider de ginasio / Equipe Rocket,
 * a partir da lista de especies disponiveis. */
static void montar_time_fixo(Treinador* t, EspeciePokemon** especies, int num_especies, int forca_extra) {
    for (int i = 0; i < 3; i++) {
        EspeciePokemon* esp = especies[rand() % num_especies];
        Pokemon* p = Pokemon_criar(esp, ap_aleatorio() + forca_extra, dp_aleatorio() + forca_extra);
        p->vertice_atual = t->vertice_atual;
        Treinador_adicionar_pokemon(t, p);
    }
}

static void imprimir_time(const Treinador* t) {
    printf("  Time de %s (xp treinador: %d, insignias: %d):\n", t->nome, t->xp, t->num_insignias_conquistadas);
    for (int i = 0; i < t->num_pokemons; i++) {
        Pokemon* p = t->pokemons[i];
        const char* estado = p->estado == CONSCIENTE ? "consciente" : (p->estado == INCONSCIENTE ? "inconsciente" : "machucado");
        printf("    - %s | HP:%d XP:%d AP:%d DP:%d [%s]\n",
               p->nome, p->hp, p->xp, Pokemon_ap_total(p), Pokemon_dp_total(p), estado);
    }
    if (t->tem_ovo) printf("    - [ovo na encubadora, progresso: %d/100]\n", t->ovo_progresso);
}

/* Processa tudo que pode acontecer quando o treinador 't' esta parado em seu
 * vertice atual (seja por ter acabado de dar um passo ate ali, seja por ja
 * estar la desde o inicio da rodada): cura no PMC, coleta de itens,
 * encontro com pokemon selvagem, desafio ao lider do ginasio, batalha
 * contra outro treinador que esteja no mesmo lugar, ataque da Equipe
 * Rocket e chance de achar um ovo. Batalhas e capturas sao proibidas no
 * laboratorio e no PMC, conforme o enunciado. */
static void processar_chegada(Mundo* m, Treinador* t, int indice_proprio) {
    Grafo* g = m->cfg->grafo;
    int v = t->vertice_atual;
    TipoVertice tipo = g->vertices[v].tipo;

    if (tipo == PMC) {
        for (int k = 0; k < t->num_pokemons; k++) {
            if (t->pokemons[k]->estado == MACHUCADO) Pokemon_curar_no_pmc(t->pokemons[k]);
        }
        return; /* batalhas e capturas sao proibidas no PMC */
    }
    if (tipo == LABORATORIO) {
        return; /* batalhas e capturas sao proibidas no laboratorio */
    }

    /* item (erva) encontrado no caminho */
    for (int i = 0; i < m->cfg->num_itens; i++) {
        if (!m->itens[i].coletado && m->itens[i].vertice == v) {
            Item_usar(t, &m->itens[i]);
            break;
        }
    }

    /* pokemon selvagem encontrado no caminho */
    for (int s = 0; s < m->cfg->num_pokemons_selvagens; s++) {
        if (!m->selvagens[s].capturado && m->selvagens[s].pokemon->vertice_atual == v) {
            int tempo_batalha = 0;
            if (Batalha_capturar_selvagem(t, m->selvagens[s].pokemon, USAR_VANTAGENS_DE_TIPO, &tempo_batalha)) {
                m->selvagens[s].capturado = 1;
            }
            m->tempo_decorrido += tempo_batalha;
            break;
        }
    }

    /* ginasio ainda nao conquistado */
    if (tipo == GINASIO) {
        int idx_ginasio = g->vertices[v].indice_insignia;
        if (!t->insignias[idx_ginasio]) {
            int tempo_batalha = 0;
            if (Batalha_treinador_vs_treinador(t, m->lideres[idx_ginasio], USAR_VANTAGENS_DE_TIPO, &tempo_batalha)) {
                Treinador_ganhar_insignia(t, idx_ginasio);
            }
            m->tempo_decorrido += tempo_batalha;
        }
    }

    /* outro treinador (ainda nao inscrito) no mesmo vertice: pode desafia-lo */
    for (int i = 0; i < m->num_treinadores; i++) {
        if (i == indice_proprio) continue;
        Treinador* outro = m->treinadores[i];
        if (outro->registrado_na_liga || outro->vertice_atual != v) continue;

        if (Batalha_desafiado_aceita(t, outro)) {
            int tempo_batalha = 0;
            Batalha_treinador_vs_treinador(t, outro, USAR_VANTAGENS_DE_TIPO, &tempo_batalha);
            m->tempo_decorrido += tempo_batalha;
        } else {
            printf("  %s recusou o desafio de %s.\n", outro->nome, t->nome);
        }
        break;
    }

    /* Equipe Rocket presente e nao escondida */
    for (int i = 0; i < NUM_AGENTES_ROCKET; i++) {
        EquipeRocket* r = m->agentes_rocket[i];
        if (!r->escondido && r->treinador->vertice_atual == v) {
            int tempo_batalha = 0;
            Rocket_tentar_roubo(r, t, g, v, USAR_VANTAGENS_DE_TIPO, &tempo_batalha);
            m->tempo_decorrido += tempo_batalha;
            break;
        }
    }

    /* chance de encontrar um ovo de pokemon selvagem no caminho */
    if (!t->tem_ovo && t->num_pokemons < MAX_POKEMONS_ATIVOS + 1 && sorteio_simples(PROB_ACHAR_OVO_NO_CAMINHO)) {
        EspeciePokemon* especie_oculta = m->cfg->especies[rand() % m->cfg->num_especies];
        if (Treinador_encontrar_ovo(t, especie_oculta, ap_aleatorio(), dp_aleatorio())) {
            printf("  %s encontrou um ovo de pokemon selvagem no caminho e o colocou na encubadora!\n", t->nome);
        }
    }
}

int main(int argc, char** argv) {
    unsigned int semente = (unsigned int)time(NULL);
    if (argc > 1) semente = (unsigned int)atoi(argv[1]); /* permite reproduzir uma execucao especifica */
    srand(semente);
    printf("Semente aleatoria: %u (passe como argumento para repetir a mesma execucao)\n\n", semente);

    /* ---------------------------------------------------------------
     * 1. Leitura do cenario a partir do arquivo texto
     * --------------------------------------------------------------- */
    const char* caminho = argc > 2 ? argv[2] : "mapa.txt";
    ConfigJogo* cfg = Config_ler_arquivo(caminho);

    printf("=== MAPA DA REGIAO ===\n");
    Grafo_imprimir(cfg->grafo);
    printf("Prazo maximo para inscricao na Liga: %ld unidades de tempo\n\n", cfg->prazo_inscricao);

    int num_ginasios;
    int* vertices_ginasio = localizar_ginasios(cfg->grafo, &num_ginasios);

    /* ---------------------------------------------------------------
     * 2. Criacao dos treinadores. Cada um pode aceitar os 3 pokemons
     *    iniciais do Prof. Carvalho (tipos distintos: agua, fogo, planta)
     *    ou recusa-los e receber apenas 1 pokemon aleatorio do laboratorio.
     * --------------------------------------------------------------- */
    Treinador** treinadores = (Treinador**)malloc(cfg->num_treinadores * sizeof(Treinador*));
    for (int i = 0; i < cfg->num_treinadores; i++) {
        char nome[50];
        snprintf(nome, sizeof(nome), "Treinador_%d", i + 1);
        treinadores[i] = Treinador_criar(nome, cfg->vertice_laboratorio, num_ginasios);

        if (cfg->num_especies > 3 && sorteio_simples(PROB_RECUSAR_STARTERS)) {
            /* recusa os 3 iniciais: recebe apenas 1 pokemon aleatorio do laboratorio */
            EspeciePokemon* especie = cfg->especies[rand() % cfg->num_especies];
            Pokemon* unico = Pokemon_criar(especie, ap_aleatorio(), dp_aleatorio());
            unico->vertice_atual = cfg->vertice_laboratorio;
            Treinador_adicionar_pokemon(treinadores[i], unico);
            printf("%s recusou os 3 iniciais e recebeu um %s aleatorio do Prof. Carvalho.\n", nome, unico->nome);
        } else {
            /* especies 0,1,2 do arquivo sao consideradas os starters (grama, fogo, agua) */
            for (int esp_idx = 0; esp_idx < 3 && esp_idx < cfg->num_especies; esp_idx++) {
                Pokemon* inicial = Pokemon_criar(cfg->especies[esp_idx], ap_aleatorio(), dp_aleatorio());
                inicial->vertice_atual = cfg->vertice_laboratorio;
                Treinador_adicionar_pokemon(treinadores[i], inicial);
            }
            printf("%s recebeu seus 3 pokemons iniciais do Prof. Carvalho.\n", nome);
        }
    }
    printf("\n");

    /* ---------------------------------------------------------------
     * 3. Lideres de ginasio (um treinador fixo por ginasio)
     * --------------------------------------------------------------- */
    Treinador** lideres = (Treinador**)malloc(num_ginasios * sizeof(Treinador*));
    for (int i = 0; i < num_ginasios; i++) {
        int v = vertices_ginasio[i];
        char nome[64];
        snprintf(nome, sizeof(nome), "Lider_%s", cfg->grafo->vertices[v].nome_ginasio);
        lideres[i] = Treinador_criar(nome, v, 0);
        montar_time_fixo(lideres[i], cfg->especies, cfg->num_especies, 5 /* lideres sao um pouco mais fortes */);
    }

    /* ---------------------------------------------------------------
     * 4. Pokemons selvagens espalhados pela regiao (posicoes/atributos aleatorios)
     * --------------------------------------------------------------- */
    PokemonSelvagem* selvagens = (PokemonSelvagem*)malloc(cfg->num_pokemons_selvagens * sizeof(PokemonSelvagem));
    for (int i = 0; i < cfg->num_pokemons_selvagens; i++) {
        EspeciePokemon* esp = cfg->especies[rand() % cfg->num_especies];
        Pokemon* p = Pokemon_criar(esp, ap_aleatorio(), dp_aleatorio());
        p->vertice_atual = vertice_aleatorio_valido(cfg);
        selvagens[i].pokemon = p;
        selvagens[i].capturado = 0;
    }

    /* ---------------------------------------------------------------
     * 5. Itens (ervas especiais) espalhados pela regiao -- requisito adicional
     * --------------------------------------------------------------- */
    ItemErva* itens = (ItemErva*)malloc((cfg->num_itens > 0 ? cfg->num_itens : 1) * sizeof(ItemErva));
    for (int i = 0; i < cfg->num_itens; i++) {
        itens[i].vertice = vertice_aleatorio_valido(cfg);
        itens[i].coletado = 0;
    }

    /* ---------------------------------------------------------------
     * 6. Equipe Rocket (item extra): mais de um agente pode rondar a regiao
     * --------------------------------------------------------------- */
    EquipeRocket** agentes_rocket = (EquipeRocket**)malloc(NUM_AGENTES_ROCKET * sizeof(EquipeRocket*));
    for (int i = 0; i < NUM_AGENTES_ROCKET; i++) {
        char nome[32];
        snprintf(nome, sizeof(nome), "Equipe_Rocket_%d", i + 1);
        agentes_rocket[i] = Rocket_criar(nome, rand() % cfg->grafo->num_vertices);
        montar_time_fixo(agentes_rocket[i]->treinador, cfg->especies, cfg->num_especies, 3);
    }

    Mundo mundo = {
        .cfg = cfg,
        .treinadores = treinadores,
        .num_treinadores = cfg->num_treinadores,
        .lideres = lideres,
        .vertices_ginasio = vertices_ginasio,
        .num_ginasios = num_ginasios,
        .selvagens = selvagens,
        .itens = itens,
        .agentes_rocket = agentes_rocket,
        .tempo_decorrido = 0
    };

    /* ---------------------------------------------------------------
     * 7. Loop principal de simulacao
     * --------------------------------------------------------------- */
    int registrados = 0;

    for (int rodada = 0; rodada < MAX_RODADAS && registrados < cfg->num_treinadores; rodada++) {
        for (int i = 0; i < cfg->num_treinadores; i++) {
            Treinador* t = treinadores[i];
            if (t->registrado_na_liga) continue;

            /* decide destino: PMC se faltam pokemons conscientes; senao o
             * proximo ginasio sem insignia; senao o estadio */
            int conscientes = Treinador_contar_conscientes(t);
            int destino;
            if (conscientes < 3) {
                destino = cfg->vertice_pmc;
            } else {
                int necessarias = num_ginasios > 8 ? 8 : num_ginasios;
                if (t->num_insignias_conquistadas < necessarias) {
                    int alvo = -1;
                    for (int gidx = 0; gidx < num_ginasios; gidx++) {
                        if (!t->insignias[gidx]) { alvo = gidx; break; }
                    }
                    destino = vertices_ginasio[alvo];
                } else {
                    destino = cfg->vertice_estadio;
                }
            }

            if (destino == cfg->vertice_pmc && t->vertice_atual == cfg->vertice_pmc) {
                /* ja esta no PMC mas ainda precisa curar: pokemons apenas
                 * "inconscientes" (nao "machucados") so recuperam HP com o
                 * tempo, entao e preciso descansar explicitamente para o
                 * tempo passar (senao ficaria parado para sempre, ja que
                 * nao ha mais nenhum deslocamento acontecendo) */
                int tempo_descanso = 30;
                for (int k = 0; k < t->num_pokemons; k++) {
                    Pokemon_recuperar_hp_por_tempo(t->pokemons[k], tempo_descanso);
                    if (t->pokemons[k]->estado == MACHUCADO) Pokemon_curar_no_pmc(t->pokemons[k]);
                }
                mundo.tempo_decorrido += tempo_descanso;
                for (int r = 0; r < NUM_AGENTES_ROCKET; r++) {
                    Rocket_atualizar_tempo(agentes_rocket[r], cfg->grafo, tempo_descanso);
                }
                if (mundo.tempo_decorrido > cfg->prazo_inscricao) break;
                continue; /* passa para o proximo treinador nesta rodada */
            }

            if (t->vertice_atual == destino) {
                /* ja esta no lugar certo: tenta agir mesmo sem se mover
                 * (ex: reencarar o lider do ginasio apos se recuperar) */
                processar_chegada(&mundo, t, i);
            } else {
                /* percorre o caminho minimo UM VERTICE POR VEZ, verificando
                 * encontros a cada parada intermediaria, nao so no destino */
                int passos_max = cfg->grafo->num_vertices + 2; /* salvaguarda contra grafos desconexos */
                while (t->vertice_atual != destino && passos_max-- > 0) {
                    int tempo_passo = Treinador_dar_passo(t, cfg->grafo, destino);
                    if (tempo_passo == 0) break; /* sem caminho possivel */
                    mundo.tempo_decorrido += tempo_passo;
                    for (int r = 0; r < NUM_AGENTES_ROCKET; r++) {
                        Rocket_atualizar_tempo(agentes_rocket[r], cfg->grafo, tempo_passo);
                    }
                    processar_chegada(&mundo, t, i);
                    if (t->registrado_na_liga) break;
                }
            }

            /* chegou ao estadio com insignias suficientes e dentro do prazo? */
            if (!t->registrado_na_liga
                && cfg->grafo->vertices[t->vertice_atual].tipo == ESTADIO
                && Treinador_pode_se_inscrever(t)
                && mundo.tempo_decorrido <= cfg->prazo_inscricao) {
                t->registrado_na_liga = 1;
                registrados++;
                printf("*** %s se inscreveu na Liga Pokemon! (tempo: %ld / prazo: %ld) ***\n",
                       t->nome, mundo.tempo_decorrido, cfg->prazo_inscricao);
            }

            if (mundo.tempo_decorrido > cfg->prazo_inscricao) break;
        }

        if (mundo.tempo_decorrido > cfg->prazo_inscricao) {
            printf("\n[Prazo esgotado em t=%ld] Treinadores que nao se inscreveram ficam inaptos.\n", mundo.tempo_decorrido);
            break;
        }
    }

    /* ---------------------------------------------------------------
     * 8. Relatorio final
     * --------------------------------------------------------------- */
    printf("\n=== RELATORIO FINAL (tempo total decorrido: %ld) ===\n", mundo.tempo_decorrido);
    for (int i = 0; i < cfg->num_treinadores; i++) {
        Treinador* t = treinadores[i];
        printf("\n%s -> %s\n", t->nome, t->registrado_na_liga ? "INSCRITO NA LIGA" : "nao se inscreveu (inapto)");
        imprimir_time(t);
    }

    int selvagens_restantes = 0;
    for (int s = 0; s < cfg->num_pokemons_selvagens; s++) if (!selvagens[s].capturado) selvagens_restantes++;
    printf("\nPokemons selvagens ainda livres na regiao: %d de %d\n", selvagens_restantes, cfg->num_pokemons_selvagens);

    int itens_restantes = 0;
    for (int i = 0; i < cfg->num_itens; i++) if (!itens[i].coletado) itens_restantes++;
    printf("Itens (ervas) ainda nao coletados: %d de %d\n", itens_restantes, cfg->num_itens);

    /* ---------------------------------------------------------------
     * 9. Liberacao de toda a memoria alocada
     * --------------------------------------------------------------- */
    for (int i = 0; i < cfg->num_treinadores; i++) Treinador_liberar(treinadores[i]);
    free(treinadores);
    for (int i = 0; i < num_ginasios; i++) Treinador_liberar(lideres[i]);
    free(lideres);
    free(vertices_ginasio);
    for (int s = 0; s < cfg->num_pokemons_selvagens; s++) {
        if (!selvagens[s].capturado) Pokemon_liberar(selvagens[s].pokemon);
    }
    free(selvagens);
    free(itens);
    for (int i = 0; i < NUM_AGENTES_ROCKET; i++) Rocket_liberar(agentes_rocket[i]);
    free(agentes_rocket);
    Config_liberar(cfg);

    return 0;
}
