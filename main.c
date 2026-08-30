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

#define USAR_VANTAGENS_DE_TIPO 1
#define MAX_RODADAS 60
#define NUM_AGENTES_ROCKET 2
#define PROB_RECUSAR_STARTERS 0.15
#define PROB_ACHAR_OVO_NO_CAMINHO 0.04
#define TEMPO_VISITA_PMC 15

typedef struct {
    Pokemon* pokemon;
    int capturado;
} PokemonSelvagem;

// junta tudo que descreve o estado do jogo, pra nao ficar passando 10 parametros pras funcoes
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

static int ap_aleatorio(void) { return 8 + rand() % 13; }
static int dp_aleatorio(void) { return 8 + rand() % 13; }
static int sorteio_simples(double probabilidade) { return ((double)rand() / RAND_MAX) < probabilidade; }

static int* localizar_ginasios(Grafo* g, int* quantidade) {
    int* lista = (int*)malloc(g->num_vertices * sizeof(int));
    int n = 0;
    for (int i = 0; i < g->num_vertices; i++) {
        if (g->vertices[i].tipo == GINASIO) lista[n++] = i;
    }
    *quantidade = n;
    return lista;
}

// vertice aleatorio que nao seja o laboratorio nem o pmc (pra posicionar selvagem/item)
static int vertice_aleatorio_valido(ConfigJogo* cfg) {
    int v;
    do { v = rand() % cfg->grafo->num_vertices; }
    while (v == cfg->vertice_laboratorio || v == cfg->vertice_pmc);
    return v;
}

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

// tudo que pode rolar quando o treinador para num vertice: pmc, item, selvagem,
// ginasio, outro treinador, rocket, achar ovo. chamada em toda parada do caminho
static void processar_chegada(Mundo* m, Treinador* t, int indice_proprio) {
    Grafo* g = m->cfg->grafo;
    int v = t->vertice_atual;
    TipoVertice tipo = g->vertices[v].tipo;

    if (tipo == PMC) {
        // trata quem ta machucado (vai descontando o tempo, nao cura na hora)
        for (int k = 0; k < t->num_pokemons; k++) {
            if (t->pokemons[k]->estado == MACHUCADO
                && Pokemon_tratar_no_pmc(t->pokemons[k], TEMPO_VISITA_PMC)) {
                printf("  [PMC] %s (%s) concluiu o tratamento e esta pronto para batalhar novamente!\n",
                       t->pokemons[k]->nome, t->nome);
            }
        }
        return; // nao rola batalha nem captura aqui
    }
    if (tipo == LABORATORIO) {
        return; // nem aqui
    }

    for (int i = 0; i < m->cfg->num_itens; i++) {
        if (!m->itens[i].coletado && m->itens[i].vertice == v) {
            Item_usar(t, &m->itens[i]);
            break;
        }
    }

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

    // cruzou com outro treinador no mesmo lugar?
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

    for (int i = 0; i < NUM_AGENTES_ROCKET; i++) {
        EquipeRocket* r = m->agentes_rocket[i];
        if (!r->escondido && r->treinador->vertice_atual == v) {
            int tempo_batalha = 0;
            Rocket_tentar_roubo(r, t, g, v, USAR_VANTAGENS_DE_TIPO, &tempo_batalha);
            m->tempo_decorrido += tempo_batalha;
            break;
        }
    }

    // chance de achar um ovo, se ainda nao tiver um
    if (!t->tem_ovo && t->num_pokemons < MAX_POKEMONS_ATIVOS + 1 && sorteio_simples(PROB_ACHAR_OVO_NO_CAMINHO)) {
        EspeciePokemon* especie_oculta = m->cfg->especies[rand() % m->cfg->num_especies];
        if (Treinador_encontrar_ovo(t, especie_oculta, ap_aleatorio(), dp_aleatorio())) {
            printf("  %s encontrou um ovo de pokemon selvagem no caminho e o colocou na encubadora!\n", t->nome);
        }
    }
}

int main(int argc, char** argv) {
    unsigned int semente = (unsigned int)time(NULL);
    if (argc > 1) semente = (unsigned int)atoi(argv[1]); // pra repetir a mesma run
    srand(semente);
    printf("Semente aleatoria: %u (passe como argumento para repetir a mesma execucao)\n\n", semente);

    const char* caminho = argc > 2 ? argv[2] : "mapa.txt";
    ConfigJogo* cfg = Config_ler_arquivo(caminho);

    printf("=== MAPA DA REGIAO ===\n");
    Grafo_imprimir(cfg->grafo);
    printf("Prazo maximo para inscricao na Liga: %ld unidades de tempo\n\n", cfg->prazo_inscricao);

    int num_ginasios;
    int* vertices_ginasio = localizar_ginasios(cfg->grafo, &num_ginasios);

    // cria os treinadores e da os pokemons iniciais (ou 1 aleatorio se recusar)
    Treinador** treinadores = (Treinador**)malloc(cfg->num_treinadores * sizeof(Treinador*));
    for (int i = 0; i < cfg->num_treinadores; i++) {
        char nome[50];
        snprintf(nome, sizeof(nome), "Treinador_%d", i + 1);
        treinadores[i] = Treinador_criar(nome, cfg->vertice_laboratorio, num_ginasios);

        if (cfg->num_especies > 3 && sorteio_simples(PROB_RECUSAR_STARTERS)) {
            // recusou os iniciais, ganha só 1 aleatorio
            EspeciePokemon* especie = cfg->especies[rand() % cfg->num_especies];
            Pokemon* unico = Pokemon_criar(especie, ap_aleatorio(), dp_aleatorio());
            unico->vertice_atual = cfg->vertice_laboratorio;
            Treinador_adicionar_pokemon(treinadores[i], unico);
            printf("%s recusou os 3 iniciais e recebeu um %s aleatorio do Prof. Carvalho.\n", nome, unico->nome);
        } else {
            // as 3 primeiras especies do arquivo sao os starters
            for (int esp_idx = 0; esp_idx < 3 && esp_idx < cfg->num_especies; esp_idx++) {
                Pokemon* inicial = Pokemon_criar(cfg->especies[esp_idx], ap_aleatorio(), dp_aleatorio());
                inicial->vertice_atual = cfg->vertice_laboratorio;
                Treinador_adicionar_pokemon(treinadores[i], inicial);
            }
            printf("%s recebeu seus 3 pokemons iniciais do Prof. Carvalho.\n", nome);
        }
    }
    printf("\n");

    // um lider fixo por ginasio, um pouco mais forte que o normal
    Treinador** lideres = (Treinador**)malloc(num_ginasios * sizeof(Treinador*));
    for (int i = 0; i < num_ginasios; i++) {
        int v = vertices_ginasio[i];
        char nome[64];
        snprintf(nome, sizeof(nome), "Lider_%s", cfg->grafo->vertices[v].nome_ginasio);
        lideres[i] = Treinador_criar(nome, v, 0);
        montar_time_fixo(lideres[i], cfg->especies, cfg->num_especies, 5);
    }

    // pokemons selvagens espalhados aleatoriamente
    PokemonSelvagem* selvagens = (PokemonSelvagem*)malloc(cfg->num_pokemons_selvagens * sizeof(PokemonSelvagem));
    for (int i = 0; i < cfg->num_pokemons_selvagens; i++) {
        EspeciePokemon* esp = cfg->especies[rand() % cfg->num_especies];
        Pokemon* p = Pokemon_criar(esp, ap_aleatorio(), dp_aleatorio());
        p->vertice_atual = vertice_aleatorio_valido(cfg);
        selvagens[i].pokemon = p;
        selvagens[i].capturado = 0;
    }

    // itens (ervas) tambem espalhados aleatoriamente
    ItemErva* itens = (ItemErva*)malloc((cfg->num_itens > 0 ? cfg->num_itens : 1) * sizeof(ItemErva));
    for (int i = 0; i < cfg->num_itens; i++) {
        itens[i].vertice = vertice_aleatorio_valido(cfg);
        itens[i].coletado = 0;
    }

    // agentes da equipe rocket, cada um com seu proprio time
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

    int registrados = 0;

    for (int rodada = 0; rodada < MAX_RODADAS && registrados < cfg->num_treinadores; rodada++) {
        for (int i = 0; i < cfg->num_treinadores; i++) {
            Treinador* t = treinadores[i];
            if (t->registrado_na_liga) continue;

            // decide onde o treinador vai nessa rodada
            int conscientes = Treinador_contar_conscientes(t);
            int destino;
            if (t->num_pokemons < 3) {
                // time incompleto: sai explorando pra achar pokemon, nao adianta ir pro pmc
                do {
                    destino = vertice_aleatorio_valido(cfg);
                } while (destino == t->vertice_atual && cfg->grafo->num_vertices > 3);
            } else if (conscientes < 3) {
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
                // ja ta no pmc mas ainda precisa curar: descansa aqui mesmo
                int tempo_descanso = 30;
                for (int k = 0; k < t->num_pokemons; k++) {
                    Pokemon_recuperar_hp_por_tempo(t->pokemons[k], tempo_descanso);
                    if (t->pokemons[k]->estado == MACHUCADO
                        && Pokemon_tratar_no_pmc(t->pokemons[k], tempo_descanso)) {
                        printf("  [PMC] %s (%s) concluiu o tratamento e esta pronto para batalhar novamente!\n",
                               t->pokemons[k]->nome, t->nome);
                    }
                }
                mundo.tempo_decorrido += tempo_descanso;
                for (int r = 0; r < NUM_AGENTES_ROCKET; r++) {
                    Rocket_atualizar_tempo(agentes_rocket[r], cfg->grafo, tempo_descanso);
                }
                if (mundo.tempo_decorrido > cfg->prazo_inscricao) break;
                continue;
            }

            if (t->vertice_atual == destino) {
                // ja ta la, so tenta agir (ex: reencarar o ginasio depois de curar)
                processar_chegada(&mundo, t, i);
            } else {
                // anda um vertice de cada vez ate chegar, checando encontro em cada parada
                int passos_max = cfg->grafo->num_vertices + 2;
                while (t->vertice_atual != destino && passos_max-- > 0) {
                    int tempo_passo = Treinador_dar_passo(t, cfg->grafo, destino);
                    if (tempo_passo == 0) break;
                    mundo.tempo_decorrido += tempo_passo;
                    for (int r = 0; r < NUM_AGENTES_ROCKET; r++) {
                        Rocket_atualizar_tempo(agentes_rocket[r], cfg->grafo, tempo_passo);
                    }
                    processar_chegada(&mundo, t, i);
                    if (t->registrado_na_liga) break;
                }
            }

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

    // libera tudo
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
