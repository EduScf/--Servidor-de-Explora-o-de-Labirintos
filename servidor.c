#include "comum.h"

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

    memset(labirinto_completo, 0, sizeof(labirinto_completo)); // Inicializa com zeros

    int i = 0, j = 0; // Contadores de linha e coluna
    char linha[128];  // Buffer para armazenar uma linha do arquivo

    while (fgets(linha, sizeof(linha), fp) && i < MAX_SIZE) {
        char *token = strtok(linha, " "); // Divide a linha em tokens usando espaço como delimitador
        j = 0; // Reseta o contador de colunas para cada nova linha
        while (token && j < MAX_SIZE) {
            labirinto_completo[i][j] = atoi(token); // Converte o token para inteiro
            token = strtok(NULL, " ");            // Pega o próximo token
            j++;
        }
        i++; // Próxima linha
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


void imprimir_labirinto_cliente(int labirinto[MAX_SIZE][MAX_SIZE], int tamanho) {
    for (int i = 0; i < tamanho; ++i) {
        for (int j = 0; j < tamanho; ++j) {
            switch (labirinto[i][j]) {
                case 0: printf("#\t"); break; // Muro
                case 1: printf("_\t"); break; // Caminho livre
                case 2: printf(">\t"); break; // Entrada
                case 3: printf("X\t"); break; // Saída
                case 4: printf("?\t"); break; // Não descoberto
                case 5: printf("+\t"); break; // Jogador
                default: printf(" \t"); break; // Espaço vazio
            }
        }
        printf("\n");
    }
}

// Função para verificar se uma célula é válida para movimento
bool posicao_valida(int x, int y) {
    return x >= 0 && x < labirinto_tamanho && y >= 0 && y < labirinto_tamanho &&
           labirinto_completo[x][y] == 1; // Caminho livre
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

    marcar_como_desconhecido();
    encontrar_posicao_inicial();  // This should set jogador_x, jogador_y and labirinto_estado[jogador_x][jogador_y] = 5
    revelar_posicoes();

    int quantidade;
    obter_movimentos_validos(resposta.moves, &quantidade);

    resposta.type = 4; // update
    memcpy(resposta.board, labirinto_estado, sizeof(labirinto_estado));

    send(csock, &resposta, sizeof(resposta), 0);
}

void processar_move(struct action *acao, int csock) {
    struct action resposta;
    memset(&resposta, 0, sizeof(resposta));

    int dx = 0, dy = 0;

    // Determinar direção com base no movimento recebido
    switch (acao->moves[0]) {
        case 1: dx = -1; break; // up
        case 2: dy = 1; break;  // right
        case 3: dx = 1; break;  // down
        case 4: dy = -1; break; // left
        default:
            resposta.type = -1; // Movimento inválido
            send(csock, &resposta, sizeof(resposta), 0);
            return;
    }

    int nx = jogador_x + dx;
    int ny = jogador_y + dy;

    // Verificar validade do movimento
    if (nx >= 0 && nx < labirinto_tamanho && ny >= 0 && ny < labirinto_tamanho &&
        (labirinto_completo[nx][ny] == 1 || labirinto_completo[nx][ny] == 3)) { // Caminho livre ou saída

        // Atualizar a posição anterior no estado do labirinto
        // CHANGED: Restore the original cell value instead of current state
        labirinto_estado[jogador_x][jogador_y] = labirinto_completo[jogador_x][jogador_y];

        // Atualizar nova posição do jogador
        jogador_x = nx;
        jogador_y = ny;
        
        // ADDED: Always mark the new player position with 5
        labirinto_estado[jogador_x][jogador_y] = 5; // Marca o jogador

        // Revelar novas posições ao redor do jogador
        revelar_posicoes();

        // Obter movimentos válidos
        int quantidade;
        obter_movimentos_validos(resposta.moves, &quantidade);

        // Preencher resposta
        resposta.type = 4; // update
        memcpy(resposta.board, labirinto_estado, sizeof(labirinto_estado));
    } else {
        // Movimento inválido
        resposta.type = -1; // Indica erro
    }

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
    printf("bound to %s, waiting for connections\n", addrstr);

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
        printf("[log] connection from %s\n", caddrstr);

        while (1) {
            struct action acao;
            memset(&acao, 0, sizeof(acao));
            size_t count = recv(csock, &acao, sizeof(acao), 0);
            if (count == 0) {
                // Conexão encerrada pelo cliente
                printf("[log] conexão encerrada por %s\n", caddrstr);
                break;
            } else if (count == -1) {
                logexit("recv");
            }

            printf("[log] received action type: %d\n", acao.type);

            // Processar comando baseado no tipo
            if (acao.type == 0) { // start
                processar_start(csock);
            } else if (acao.type == 1) { // move
                processar_move(&acao, csock);
            } else if (acao.type == 7) { // exit
                printf("[log] cliente %s encerrou a conexão.\n", caddrstr);
                break;
            } else {
                // Comando desconhecido
                struct action resposta;
                memset(&resposta, 0, sizeof(resposta));
                resposta.type = -1; // Comando inválido
                count = send(csock, &resposta, sizeof(resposta), 0);
                if (count != sizeof(resposta)) {
                    logexit("send");
                }
            }
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}

