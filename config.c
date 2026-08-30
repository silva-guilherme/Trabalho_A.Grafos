#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

static void erro_fatal(const char* msg) {
    fprintf(stderr, "Erro ao ler arquivo de configuracao: %s\n", msg);
    exit(1);
}


static int ler_token(FILE* f, char* buf, int tamanho) {
    (void)tamanho; 
    int c;
    while (1) {
        c = fgetc(f);
        if (c == EOF) return 0;
        if (c == '#') {
            while (c != '\n' && c != EOF) c = fgetc(f);
            continue;
        }
        if (isspace(c)) continue;
        ungetc(c, f);
        break;
    }
    return fscanf(f, "%s", buf) == 1;
}

static void esperar_palavra(FILE* f, const char* esperado) {
    char buf[100];
    if (!ler_token(f, buf, sizeof(buf)) || strcmp(buf, esperado) != 0) {
        fprintf(stderr, "Esperava '%s' mas encontrei '%s'\n", esperado, buf);
        exit(1);
    }
}


static int ler_inteiro(FILE* f) {
    char buf[32];
    if (!ler_token(f, buf, sizeof(buf))) erro_fatal("valor inteiro ausente");
    return atoi(buf);
}

static int ler_inteiro_apos(FILE* f, const char* palavra_chave) {
    esperar_palavra(f, palavra_chave);
    return ler_inteiro(f);
}


static EspeciePokemon* parsear_especie(const char* linha) {
    char copia[200];
    strncpy(copia, linha, 199);
    copia[199] = '\0';

    char* parte_nomes = copia;
    char* parte_tipos = strchr(copia, '|');
    if (parte_tipos == NULL) erro_fatal("linha de especie sem '|' separando tipos");
    *parte_tipos = '\0';
    parte_tipos++;

    const char* nomes[3];
    int num_fases = 0;
    char* tok = strtok(parte_nomes, ",");
    while (tok != NULL && num_fases < 3) {
        if (strcmp(tok, "-") != 0) {
            nomes[num_fases++] = tok;
        }
        tok = strtok(NULL, ",");
    }

    char tipo1_str[30], tipo2_str[30] = "";
    char* barra = strchr(parte_tipos, '/');
    if (barra != NULL) {
        *barra = '\0';
        strncpy(tipo1_str, parte_tipos, 29); tipo1_str[29] = '\0';
        strncpy(tipo2_str, barra + 1, 29); tipo2_str[29] = '\0';
        
        tipo2_str[strcspn(tipo2_str, "\r\n")] = '\0';
    } else {
        strncpy(tipo1_str, parte_tipos, 29); tipo1_str[29] = '\0';
        tipo1_str[strcspn(tipo1_str, "\r\n")] = '\0';
    }

    TipoPokemon t1 = Tipo_from_string(tipo1_str);
    int tem_t2 = strlen(tipo2_str) > 0;
    TipoPokemon t2 = tem_t2 ? Tipo_from_string(tipo2_str) : t1;

    return Especie_criar(nomes, num_fases, t1, tem_t2, t2);
}

ConfigJogo* Config_ler_arquivo(const char* caminho_arquivo) {
    FILE* f = fopen(caminho_arquivo, "r");
    if (f == NULL) erro_fatal("nao foi possivel abrir o arquivo");

    ConfigJogo* cfg = (ConfigJogo*)malloc(sizeof(ConfigJogo));

    
    int num_vertices = ler_inteiro_apos(f, "NUM_VERTICES");
    int num_arestas = ler_inteiro_apos(f, "NUM_ARESTAS");
    cfg->grafo = Grafo_criar(num_vertices);
    for (int i = 0; i < num_arestas; i++) {
        int a = ler_inteiro(f);
        int b = ler_inteiro(f);
        int peso = ler_inteiro(f);
        Grafo_adicionar_aresta_bidirecional(cfg->grafo, a, b, peso);
    }

    
    cfg->vertice_laboratorio = ler_inteiro_apos(f, "LABORATORIO");
    cfg->vertice_pmc = ler_inteiro_apos(f, "PMC");
    cfg->vertice_estadio = ler_inteiro_apos(f, "ESTADIO");
    Grafo_definir_tipo(cfg->grafo, cfg->vertice_laboratorio, LABORATORIO);
    Grafo_definir_tipo(cfg->grafo, cfg->vertice_pmc, PMC);
    Grafo_definir_tipo(cfg->grafo, cfg->vertice_estadio, ESTADIO);

    int num_ginasios = ler_inteiro_apos(f, "NUM_GINASIOS");
    for (int i = 0; i < num_ginasios; i++) {
        esperar_palavra(f, "GINASIO");
        int vertice = ler_inteiro(f);
        char nome[50];
        if (!ler_token(f, nome, sizeof(nome))) erro_fatal("nome de ginasio ausente");
        Grafo_definir_ginasio(cfg->grafo, vertice, nome, i);
    }

   
    cfg->num_especies = ler_inteiro_apos(f, "NUM_ESPECIES");
    cfg->especies = (EspeciePokemon**)malloc(cfg->num_especies * sizeof(EspeciePokemon*));
    for (int i = 0; i < cfg->num_especies; i++) {
        char linha[200];
        if (!ler_token(f, linha, sizeof(linha))) erro_fatal("linha de especie ausente");
        cfg->especies[i] = parsear_especie(linha);
    }

    
    cfg->num_pokemons_selvagens = ler_inteiro_apos(f, "NUM_POKEMONS");
    cfg->num_treinadores = ler_inteiro_apos(f, "NUM_TREINADORES");
    cfg->num_itens = ler_inteiro_apos(f, "NUM_ITENS");

    fclose(f);

    
    long soma = cfg->grafo->soma_pesos;
    long minimo = soma * 10, maximo = soma * 15;
    cfg->prazo_inscricao = minimo + (maximo > minimo ? rand() % (maximo - minimo + 1) : 0);

    return cfg;
}

void Config_liberar(ConfigJogo* cfg) {
    Grafo_liberar(cfg->grafo);
    for (int i = 0; i < cfg->num_especies; i++) free(cfg->especies[i]);
    free(cfg->especies);
    free(cfg);
}
