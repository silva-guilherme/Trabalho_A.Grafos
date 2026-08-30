# Rumo à Liga Pokémon — Projeto de Algoritmos em Grafos

## Sobre o projeto

Simulação em C de uma "jornada pokémon" pelo mapa de uma região, feita para
a disciplina de Algoritmos em Grafos. O mapa é modelado como um grafo
ponderado (os pesos representam tempo de percurso), e os treinadores se
movem pelo grafo usando **Dijkstra com heap binário** para achar o caminho
mais rápido até seus objetivos: capturar pokémons selvagens, treinar,
desafiar líderes de ginásio e, por fim, se inscrever na Liga antes do prazo.
Além da dinâmica principal (captura, evolução, XP, HP, batalhas 3x3), o
projeto implementa dois itens extras do enunciado: vantagens de tipo entre
pokémons e uma Equipe Rocket que rouba pokémons/insígnias.

Todo o código é em C puro, sem bibliotecas externas de estrutura de dados -
a lista de adjacência, o heap do Dijkstra, etc.

## Requisitos para compilar

É preciso apenas de um compilador C (`gcc`) e do utilitário `make`. Nenhuma
biblioteca externa é necessária além da libc padrão e `libm` (matemática),
que já vêm com qualquer instalação do gcc.

### Linux

A maioria das distribuições já vem com `gcc`/`make`, ou eles fazem parte do
pacote de ferramentas de compilação. Para instalar, se necessário:

```bash
# Ubuntu / Debian
sudo apt update && sudo apt install build-essential

# Fedora
sudo dnf install gcc make

# Arch
sudo pacman -S gcc make
```

Depois é só rodar `make` na pasta do projeto (veja a seção seguinte).

### Windows

O projeto usa um `Makefile` e comandos no estilo Unix, então o caminho mais
simples é usar o **WSL** (Subsistema do Windows para Linux), que roda um
Linux de verdade dentro do Windows:

1. Abra o PowerShell como administrador e rode `wsl --install` (instala o
   WSL com Ubuntu por padrão). Reinicie o computador se for pedido.
2. Abra o "Ubuntu" que apareceu no menu iniciar.
3. Dentro do Ubuntu, instale as ferramentas de compilação:
   ```bash
   sudo apt update && sudo apt install build-essential
   ```
4. Copie a pasta do projeto para dentro do WSL (ou acesse os arquivos do
   Windows pelo caminho `/mnt/c/...`) e siga a seção "Como compilar e
   executar" normalmente, como se fosse Linux.

Se preferir não usar o WSL, dá para compilar nativamente no Windows com o
**MSYS2**:

1. Baixe e instale o MSYS2 em <https://www.msys2.org/>.
2. Abra o terminal "MSYS2 MinGW64" e instale o compilador:
   ```bash
   pacman -S mingw-w64-x86_64-gcc make
   ```
3. Navegue até a pasta do projeto nesse mesmo terminal e rode `make`
   normalmente. O executável gerado será `pokemon_liga.exe`.

(Visual Studio sozinho não compila este projeto direto, pois o `Makefile` e
o código assumem um compilador estilo gcc/Unix - use uma das duas opções
acima.)

## Como compilar e executar

```bash
make            # compila (gcc, -Wall -Wextra -std=c11)
./pokemon_liga [semente] [arquivo_mapa.txt]
```

No Windows (WSL ou MSYS2), os comandos são os mesmos; só no MSYS2 o
executável chama `./pokemon_liga.exe` em vez de `./pokemon_liga`.

- `semente` (opcional): inteiro para reproduzir exatamente a mesma execução
  (o programa usa números aleatórios para AP/DP iniciais, esquiva, crítico,
  posições de pokémons/itens selvagens etc). Se omitido, usa a hora atual.
- `arquivo_mapa.txt` (opcional): caminho do arquivo de cenário. Padrão: `mapa.txt`.

Exemplo: `./pokemon_liga 42 mapa.txt`

`make clean` remove os binários. `make run` compila e já executa.

Se não tiver `make` disponível por algum motivo, dá para compilar direto
com o gcc, sem o Makefile:

```bash
gcc -Wall -Wextra -std=c11 -o pokemon_liga grafo.c tipos.c pokemon.c treinador.c batalha.c rocket.c config.c item.c main.c -lm
```

## Formato do arquivo de mapa (`mapa.txt`)

```
NUM_VERTICES <n>
NUM_ARESTAS <m>
<origem> <destino> <peso>     # m linhas; cada uma é adicionada nos DOIS sentidos
LABORATORIO <vertice>
PMC <vertice>
ESTADIO <vertice>
NUM_GINASIOS <g>
GINASIO <vertice> <nome>      # g linhas
NUM_ESPECIES <e>
Fase1,Fase2,Fase3|TIPO1/TIPO2 # e linhas; use "-" para fase inexistente e omita TIPO2 se só houver 1 tipo
NUM_POKEMONS <p>              # pokémons selvagens espalhados pela região
NUM_TREINADORES <t>
NUM_ITENS <i>                 # ervas especiais espalhadas pela região
```

Linhas começando com `#` são comentários e são ignoradas. As 3 primeiras
espécies listadas são usadas como os starters (água/fogo/planta) entregues
pelo Prof. Carvalho; as demais aparecem apenas como pokémons selvagens/ovos.

O prazo de inscrição na Liga é sorteado automaticamente entre 10x e 15x a
soma de todos os pesos das arestas, como exigido no enunciado.

## Arquitetura (módulos)

| Arquivo             | Responsabilidade                                                        |
|---------------------|--------------------------------------------------------------------------|
| `grafo.h/.c`         | Grafo ponderado, lista de adjacência duplamente encadeada, **Dijkstra com heap binário** (O((V+E) log V)) para caminho mínimo/mais distante |
| `tipos.h/.c`         | Tipos de pokémon e tabela de vantagens/desvantagens (item extra)         |
| `pokemon.h/.c`       | Pokémon: HP, XP, AP/DP, evolução, recuperação de HP, estados             |
| `treinador.h/.c`     | Treinador: time (máx. 6), ovos/incubadora, insígnias, movimentação vértice a vértice |
| `batalha.h/.c`       | Duelo pokémon a pokémon, batalha 3x3 entre treinadores, captura selvagem, aceite/desistência |
| `item.h/.c`          | Ervas especiais que curam +10 HP de todo o time consciente               |
| `rocket.h/.c`        | Equipe Rocket (item extra): rouba pokémons/insígnias, foge e reaparece   |
| `config.h/.c`        | Leitura do arquivo texto (grafo + cenário)                               |
| `main.c`             | Monta o cenário e roda o laço de simulação                              |

## Complexidade dos algoritmos

- **Dijkstra com heap binário**: O((V + E) log V) — usado para calcular o
  caminho mínimo (tempo) de um treinador até seu próximo destino (ginásio,
  PMC ou estádio) e para achar o vértice mais distante (Equipe Rocket).
  Preferido a uma busca linear ingênua O(V²) por ser assintoticamente melhor
  em grafos esparsos, que é o caso típico de um mapa de região.
- Percurso/impressão do grafo: O(V + E).
- Duelo pokémon a pokémon: O(1) por turno; o duelo inteiro é limitado a um
  número máximo de turnos para garantir término mesmo em situações onde
  nenhum dos lados consegue causar dano.

## Requisitos cobertos pela simulação

- Leitura do grafo ponderado e do cenário a partir de arquivo texto.
- Movimentação **um vértice por vez** (`Treinador_dar_passo`), com encontros
  (pokémons selvagens, itens, outros treinadores, ginásios, Equipe Rocket)
  verificados em **cada parada intermediária** do caminho, não só no destino
  final.
- Três pokémons iniciais de tipos distintos (água/fogo/planta), com opção de
  o treinador recusá-los e receber apenas 1 pokémon aleatório do laboratório.
- Limite de 6 pokémons ativos + no máximo 1 ovo não eclodido (total ≤ 7);
  excedente é enviado ao Prof. Carvalho.
- Ovos: encontrados no caminho (chance por passo), eclodem após 100 unidades
  de distância percorrida na encubadora.
- XP por vitória/derrota em batalha, XP por distância percorrida (1 a cada
  100 unidades), evolução a cada 1000 XP acumulados (+30% AP/DP), bônus de
  +1 AP/DP ao vencer oponente com XP ≥ o seu.
- HP de 1 a 100, recuperação de +1 a cada 10 unidades de tempo, estados
  consciente/inconsciente/machucado, tratamento no PMC, cura por erva
  (apenas em pokémons conscientes).
- Batalhas 3x3 entre treinadores (contra líderes de ginásio, contra outros
  treinadores encontrados no caminho e contra a Equipe Rocket), com
  substituição de pokémon inconsciente, esquiva e crítico proporcionais à
  diferença de XP, e o desafiado podendo **aceitar ou recusar** o duelo.
- Captura de pokémons selvagens, com possibilidade de o treinador
  **desistir** antes de lutar (mais provável quanto mais forte o selvagem
  parece).
- Batalhas e capturas são **proibidas no laboratório e no PMC**.
- Insígnias acumulativas e permanentes; inscrição na Liga exige 8 insígnias
  (ou todas, se houver ≤ 8 ginásios) dentro do prazo sorteado; passado o
  prazo, o treinador fica inapto mesmo com insígnias suficientes.

## Item extra implementado: vantagens de tipo

Cada pokémon tem 1 ou 2 tipos (18 tipos possíveis, conforme a franquia
original). O multiplicador de dano de um ataque é calculado combinando a
vantagem contra cada tipo do defensor (multiplicação dos fatores, igual ao
jogo original): 2x (super efetivo), 0.5x (pouco efetivo), 0x (imune) ou 1x
(neutro). Ver `tipos.c` para a tabela completa. Pode ser desligado trocando
`USAR_VANTAGENS_DE_TIPO` para `0` em `main.c`.

## Item extra implementado: Equipe Rocket

Modelada como um "treinador" especial (`rocket.h/.c`) com time próprio.
Por padrão rondam `NUM_AGENTES_ROCKET` (2) agentes simultâneos, configurável
em `main.c`. A cada encontro com um treinador (mesmo vértice), desafia-o
para um duelo 3x3. Se vencer, rouba um pokémon aleatório (ou uma insígnia,
se o alvo não tiver pokémon sobressalente) e foge, ficando invisível por um
tempo aleatório antes de reaparecer em um vértice qualquer. Se perder, é
enviada para o vértice mais distante do ponto do ataque (via Dijkstra).

## Divisão entre os 3 integrantes do grupo


1. **Guilherme — Grafo e movimentação**: `grafo.h/.c` (lista de adjacência,
   Dijkstra com heap, reconstrução de caminho) + `config.h/.c` (leitura do
   arquivo) + a lógica de movimentação em `treinador.c` (`Treinador_dar_passo`).
2. **Ytallo — Pokémon e batalhas**: `pokemon.h/.c` (XP, evolução, HP,
   estados) + `batalha.h/.c` (duelo, batalha 3x3, captura, aceite/desistência)
   + `tipos.h/.c` (vantagens de tipo).
3. **Lucas — Treinador e simulação/itens extras**: `treinador.h/.c`
   (time, ovos/incubadora, insígnias) + `item.h/.c` (ervas) + `rocket.h/.c`
   (Equipe Rocket) + `main.c` (orquestração da simulação, geração aleatória
   de entidades).

