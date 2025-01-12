#include "common.h"

//Coloque as bibliotecas padrões de C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//Armazenar o labirinto: Ler o arquivo de entrada de in.txt e salvar o labirinto completo e o estado atual em matrizes separadas
#define MAX_SIZE 10

int labirinto_completo[MAX_SIZE][MAX_SIZE];
int labirinto_estado[MAX_SIZE][MAX_SIZE];
int jogador_x, jogador_y; // Posição atual do jogador
int labirinto_tamanho;    // Tamanho do labirinto (assumido quadrado)

struct action {
    int type;
    int moves[100];
    int board[10][10];
};

// Função para carregar o labirinto de um arquivo
void carregar_labirinto(const char *arquivo) {
    FILE *fp = fopen(arquivo, "r");
    if (!fp) {
        perror("Erro ao abrir o arquivo de labirinto");
        exit(EXIT_FAILURE);
    }

    // Inicializa toda a matriz com zeros
    memset(labirinto_completo, 0, sizeof(labirinto_completo));

    int i = 0; // Contador de linha
    char linha[128];  // Buffer para armazenar uma linha do arquivo

    while (fgets(linha, sizeof(linha), fp) && i < MAX_SIZE) {
        char *token = strtok(linha, " "); 
        int j = 0; // Contador de coluna
        while (token && j < MAX_SIZE) {
            labirinto_completo[i][j] = atoi(token);
            token = strtok(NULL, " ");
            j++;
        }
        i++;
    }

    labirinto_tamanho = i; // Define o número de linhas lidas
    fclose(fp);
}

void encontrar_posicao_inicial() {
    for (int i = 0; i < labirinto_tamanho; ++i) {
        for (int j = 0; j < labirinto_tamanho; ++j) {
            if (labirinto_completo[i][j] == 2) { // Entrada
                jogador_x = i;
                jogador_y = j;
                labirinto_estado[i][j] = 5; // Marca jogador ('+')
                return; // Finaliza ao encontrar a entrada
            }
        }
    }

    fprintf(stderr, "Erro: entrada (2) não encontrada no labirinto\n");
    exit(EXIT_FAILURE);
}

void marcar_como_desconhecido() {
    for (int i = 0; i < labirinto_tamanho; ++i) {
        for (int j = 0; j < labirinto_tamanho; ++j) {
            labirinto_estado[i][j] = 4; // Todas as posições são marcadas como '?'
        }
    }
}

void revelar_posicoes() {
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            int nx = jogador_x + dx;
            int ny = jogador_y + dy;

            // Verificar limites do labirinto
            if (nx >= 0 && nx < labirinto_tamanho && ny >= 0 && ny < labirinto_tamanho) {
                // Revelar a posição se for desconhecida
                // Preserve a entrada (>)
                if (labirinto_completo[nx][ny] == 2) {
                    labirinto_estado[nx][ny] = 2; // Preserva o símbolo de entrada
                } else {
                    labirinto_estado[nx][ny] = labirinto_completo[nx][ny];
                }
            }
        }
    }

    // Marca a posição atual do jogador como '+', exceto na entrada
    if (labirinto_completo[jogador_x][jogador_y] != 2) {
        labirinto_estado[jogador_x][jogador_y] = 5;
    }
}


void imprimir_labirinto(int board[10][10]) {
    //printf("Estado do labirinto:\n");
    for (int i = 0; i < labirinto_tamanho; ++i) {
        for (int j = 0; j < labirinto_tamanho; ++j) {
            switch (board[i][j]) {
                case 0: printf("# "); break; // Muro
                case 1: printf("_ "); break; // Caminho livre
                case 2: printf("> "); break; // Entrada
                case 3: printf("X "); break; // Saída
                case 4: printf("? "); break; // Não descoberto
                case 5: printf("+ "); break; // Jogador
                default: printf("  "); break; // Espaço vazio
            }
        }
        printf("\n");
    }
}

// Função para verificar se uma célula é válida para movimento
bool posicao_valida(int x, int y) {
    return x >= 0 && x < labirinto_tamanho && 
           y >= 0 && y < labirinto_tamanho &&
           (labirinto_completo[x][y] == 1 || // Caminho livre
            labirinto_completo[x][y] == 3 || // Saída
            labirinto_completo[x][y] == 2);  // Entrada
}

// Função para determinar os movimentos válidos
void obter_movimentos_validos(int *movimentos, int *quantidade) {
    *quantidade = 0; // Zera a quantidade de movimentos

    // Verificar cada direção
    if (posicao_valida(jogador_x - 1, jogador_y)) { // Cima
        movimentos[*quantidade] = 1;
        (*quantidade)++;
    }
    if (posicao_valida(jogador_x, jogador_y + 1)) { // Direita
        movimentos[*quantidade] = 2;
        (*quantidade)++;
    }
    if (posicao_valida(jogador_x + 1, jogador_y)) { // Baixo
        movimentos[*quantidade] = 3;
        (*quantidade)++;
    }
    if (posicao_valida(jogador_x, jogador_y - 1)) { // Esquerda
        movimentos[*quantidade] = 4;
        (*quantidade)++;
    }
}

void processar_start(int csock) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    // Reiniciar o estado do labirinto
    marcar_como_desconhecido();
    encontrar_posicao_inicial();
    revelar_posicoes();

    int quantidade;
    obter_movimentos_validos(resposta.moves, &quantidade);

    resposta.type = 4; // update
    memcpy(resposta.board, labirinto_estado, sizeof(labirinto_estado));

    send(csock, &resposta, sizeof(resposta), 0);
}

void processar_move(struct action *acao, int csock, bool jogo_iniciado) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    if (!jogo_iniciado) {
        resposta.type = -2; // Jogo não iniciado
        send(csock, &resposta, sizeof(resposta), 0);
        return;
    }

    int dx = 0, dy = 0;

    // Determinar direção com base no movimento recebido
    switch (acao->moves[0]) {
        case 1: dx = -1; break; // up
        case 2: dy = 1; break;  // right
        case 3: dx = 1; break;  // down
        case 4: dy = -1; break; // left
        default:
            resposta.type = -1; // Movimento inválido
            size_t count = send(csock, &resposta, sizeof(resposta), 0);
            if (count != sizeof(resposta)) {
                logexit("send");
            }
            return;
    }

    int nx = jogador_x + dx;
    int ny = jogador_y + dy;

    // Verificar validade do movimento
    if (nx >= 0 && nx < labirinto_tamanho && ny >= 0 && ny < labirinto_tamanho &&
        (labirinto_completo[nx][ny] == 1 || labirinto_completo[nx][ny] == 2 || labirinto_completo[nx][ny] == 3)) { // Caminho livre, entrada ou saída

        // Atualizar a posição anterior no estado do labirinto
        labirinto_estado[jogador_x][jogador_y] = labirinto_completo[jogador_x][jogador_y];

        // Atualizar nova posição do jogador
        jogador_x = nx;
        jogador_y = ny;
        
        // Sempre marcar a nova posição do jogador
        labirinto_estado[jogador_x][jogador_y] = 5; 

        // Revelar novas posições ao redor do jogador
        revelar_posicoes();

        // Verificar se chegou na saída
        if (labirinto_completo[jogador_x][jogador_y] == 3) {
            //printf("Jogador chegou na saída!\n");
            
            // Copiar o labirinto completo para o estado do labirinto
            for (int i = 0; i < labirinto_tamanho; ++i) {
                for (int j = 0; j < labirinto_tamanho; ++j) {
                    resposta.board[i][j] = labirinto_completo[i][j];
                }
            }
            resposta.type = 5; // Tipo de ação de vitória
        } else {
            // Obter movimentos válidos
            int quantidade;
            obter_movimentos_validos(resposta.moves, &quantidade);

            // Preencher resposta
            resposta.type = 4; // update
            memcpy(resposta.board, labirinto_estado, sizeof(labirinto_estado));
        }
    } else {
        resposta.type = -3; // Movimento não permitido
        send(csock, &resposta, sizeof(resposta), 0);
        return;
    }

    // Enviar resposta ao cliente
    size_t count = send(csock, &resposta, sizeof(resposta), 0);
    if (count != sizeof(resposta)) {
        logexit("send");
    }
}

void processar_map(int csock) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    // Copiar o estado atual do labirinto para a resposta
    for (int i = 0; i < labirinto_tamanho; ++i) {
        for (int j = 0; j < labirinto_tamanho; ++j) {
            resposta.board[i][j] = labirinto_estado[i][j];
        }
    }

    // Garantir que o jogador está marcado corretamente
    resposta.board[jogador_x][jogador_y] = 5; // Marca posição do jogador como '+'

    resposta.type = 4; // Envia o estado atualizado do labirinto

    // Enviar o estado do labirinto para o cliente
    size_t count = send(csock, &resposta, sizeof(resposta), 0);
    if (count != sizeof(resposta)) {
        logexit("send");
    }
}

void processar_reset(int csock) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    // Reiniciar o estado do labirinto
    marcar_como_desconhecido();
    encontrar_posicao_inicial(); // Define jogador_x e jogador_y
    revelar_posicoes();          // Atualiza as posições visíveis

    // Obter movimentos válidos
    int quantidade;
    obter_movimentos_validos(resposta.moves, &quantidade);

    resposta.type = 6; // Tipo de resposta para reset é um novo type = 6
    memcpy(resposta.board, labirinto_estado, sizeof(labirinto_estado));

    // Enviar o estado inicial ao cliente
    size_t count = send(csock, &resposta, sizeof(resposta), 0);
    if (count != sizeof(resposta)) {
        logexit("send");
    }

    printf("starting new game\n");
}

void processar_hint(int csock, bool jogo_iniciado) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    // Verificar se o jogo foi iniciado
    if (!jogo_iniciado) {
        resposta.type = -2; // Jogo não iniciado
        send(csock, &resposta, sizeof(resposta), 0);
        return;
    }

    int movimentos[100] = {0}; // Inicializa o vetor de movimentos com zeros
    int movimento_atual = 0;
    int visitados[MAX_SIZE][MAX_SIZE] = {0}; // Matriz para evitar revisitar posições
    int x = jogador_x, y = jogador_y;

    visitados[x][y] = 1; // Marca a posição inicial como visitada

    while (labirinto_completo[x][y] != 3) { // Enquanto não atingir a saída
        if (movimento_atual >= 99) { // Evitar ultrapassar o limite de 100 movimentos
            break;
        }

        // Verificar direções na ordem da regra da mão direita: direita, baixo, esquerda, cima
        if (y + 1 < labirinto_tamanho && labirinto_completo[x][y + 1] != 0 && !visitados[x][y + 1]) {
            movimentos[movimento_atual++] = 2; // right
            y += 1;
        } else if (x + 1 < labirinto_tamanho && labirinto_completo[x + 1][y] != 0 && !visitados[x + 1][y]) {
            movimentos[movimento_atual++] = 3; // down
            x += 1;
        } else if (y - 1 >= 0 && labirinto_completo[x][y - 1] != 0 && !visitados[x][y - 1]) {
            movimentos[movimento_atual++] = 4; // left
            y -= 1;
        } else if (x - 1 >= 0 && labirinto_completo[x - 1][y] != 0 && !visitados[x - 1][y]) {
            movimentos[movimento_atual++] = 1; // up
            x -= 1;
        } else {
            break; // Sem opções válidas (evita loops em labirintos desconexos)
        }

        visitados[x][y] = 1; // Marca a nova posição como visitada
    }

    // Preenche os movimentos na resposta
    memcpy(resposta.moves, movimentos, sizeof(movimentos));
    resposta.type = 4; // Tipo de atualização para dica

    // Enviar resposta ao cliente
    size_t count = send(csock, &resposta, sizeof(resposta), 0);
    if (count != sizeof(resposta)) {
        logexit("send");
    }
}

#define BUFSZ 1024
void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server ports>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 5 || strcmp(argv[3], "-i") != 0) {
        usage(argc, argv);
    }

    // Carregar o labirinto na inicialização
    carregar_labirinto(argv[4]);       // Carregar o labirinto do arquivo
    marcar_como_desconhecido();       // Inicializar estado com '?'
    bool jogo_iniciado = false;       // Estado do jogo não iniciado ainda
    bool jogo_vencido = false;        // Estado do jogo não vencido ainda
    
    // Configuração do socket do servidor
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)&storage;
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    //printf("bound to %s, waiting for connections\n", addrstr); //Retirado pois o professor não quer nesse formato

    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)&cstorage;
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }
        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        //printf("[log] connection from %s\n", caddrstr); //Retirado pois o professor não quer nesse formato
        printf("client connected\n");

        while (1) {
            struct action acao;
            memset(&acao, 0, sizeof(acao));
            size_t count = recv(csock, &acao, sizeof(acao), 0);
            if (count == 0) {
                // Conexão encerrada pelo cliente
                //printf("[log] conexão encerrada por %s\n", caddrstr);
                printf("client disconnected\n");
                break;
            } else if (count == -1) {
                logexit("recv");
            }

            //printf("[log] received action type: %d\n", acao.type); // Retirado pois o professor não quer nesse formato

            // Controle pós-vitória
            if (jogo_vencido) {
                if (acao.type == 6) { // Reset
                    jogo_vencido = false;
                    jogo_iniciado = true;
                    processar_reset(csock);
                } else if (acao.type == 7) { // Exit
                    printf("client disconnected\n");
                    break;
                } else {
                    // Responder erro para comandos inválidos
                    struct action resposta;
                    memset(&resposta, 0, sizeof(resposta));
                    resposta.type = -2; // Inicie uma partida nova
                    send(csock, &resposta, sizeof(resposta), 0);
                } 
                continue; // Pula para o próximo comando
            }

            // Verificar estado do jogo antes de aceitar comandos específicos
           if (!jogo_iniciado && (acao.type == 1 || acao.type == 2 || acao.type == 3 || acao.type == 6)) {
                struct action resposta;
                memset(&resposta, 0, sizeof(resposta));
                resposta.type = -2; // Erro: jogo não iniciado
                send(csock, &resposta, sizeof(resposta), 0);
                continue;
            }

            // Processar comando baseado no tipo
            if (acao.type == 0) { // start
                jogo_iniciado = true;
                processar_start(csock);
                printf("starting new game\n");
            } else if (!jogo_iniciado) {
                // Ignorar comandos inválidos antes do start
                struct action resposta;
                memset(&resposta, 0, sizeof(resposta));
                resposta.type = -1; // Erro
                send(csock, &resposta, sizeof(resposta), 0);
            } else if (acao.type == 1) { // move
                processar_move(&acao, csock, jogo_iniciado);
                if (labirinto_completo[jogador_x][jogador_y] == 3) {
                    jogo_vencido = true;
                }
            } else if (acao.type == 2) { // map
                processar_map(csock);
            } else if (acao.type == 3) { // hint
                processar_hint(csock, jogo_iniciado);
            } else if (acao.type == 6) { // reset
                processar_reset(csock);
            } else if (acao.type == 7) { // exit
                //printf("[log] cliente %s encerrou a conexão.\n", caddrstr);
                printf("client disconnected\n");
                break;
            } else {
                // Comando desconhecido
                struct action resposta;
                memset(&resposta, 0, sizeof(resposta));
                resposta.type = -1; // Comando inválido
                send(csock, &resposta, sizeof(resposta), 0);
            }
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}