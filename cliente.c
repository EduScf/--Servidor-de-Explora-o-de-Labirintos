#include "comum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFSZ 1024

struct action {
    int type;
    int moves[100];
    int board[10][10];
};

void usage(int argc, char **argv) {
    printf("usage: %s <server_ip> <server_port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void imprimir_labirinto(int board[10][10]) {
    printf("Estado do labirinto:\n");
    // Find the actual size of the maze
    int tamanho = 0;
    for (int i = 0; i < 10; i++) {
        int linha_vazia = 1;
        for (int j = 0; j < 10; j++) {
            if (board[i][j] != 0 && board[i][j] != -1) {
                linha_vazia = 0;
                break;
            }
        }
        if (linha_vazia) {
            tamanho = i;
            break;
        }
    }

    // If no empty line found, use full size
    if (tamanho == 0) {
        tamanho = 10;
    }

    for (int i = 0; i < tamanho; ++i) {
        for (int j = 0; j < tamanho; ++j) {
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

//A função imprimirLabirintoVitoria vai imprimir o labirinto completo, com todas as posições reveladas, considerando que podemos receber tabuleiros 5x5, 6x6,..., 10x10 devemos interpretar o tamanho correto
void imprimirLabirintoVitoria(int board[10][10]) {
    printf("Labirinto completo revelado:\n");
    // Find the actual size of the maze
    int tamanho = 0;
    for (int i = 0; i < 10; i++) {
        int linha_vazia = 1;
        for (int j = 0; j < 10; j++) {
            if (board[i][j] != 0 && board[i][j] != -1) {
                linha_vazia = 0;
                break;
            }
        }
        if (linha_vazia) {
            tamanho = i;
            break;
        }
    }

    // If no empty line found, use full size
    if (tamanho == 0) {
        tamanho = 10;
    }

    // Increment tamanho for "gambiarra"
    if (tamanho < 10) {
        tamanho++;
    }

    for (int i = 0; i < tamanho; ++i) {
        for (int j = 0; j < tamanho; ++j) {
            if (i < 10 && j < 10) { // Prevent out-of-bound access
                switch (board[i][j]) {
                    case 0: printf("# "); break; // Muro
                    case 1: printf("_ "); break; // Caminho livre
                    case 2: printf("> "); break; // Entrada
                    case 3: printf("X "); break; // Saída
                    case 4: printf("? "); break; // Não descoberto
                    case 5: printf("+ "); break; // Jogador
                    default: printf("  "); break; // Espaço vazio
                }
            } else {
                printf("  "); // Preenche posições inexistentes
            }
        }
        printf("\n");
    }
}


void imprimir_movimentos(int moves[100]) {
    printf("Movimentos válidos: ");
    for (int i = 0; moves[i] != 0; ++i) {
        switch (moves[i]) {
            case 1: printf("up "); break;
            case 2: printf("right "); break;
            case 3: printf("down "); break;
            case 4: printf("left "); break;
        }
    }
    printf("\n");
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (addrparse(argv[1], argv[2], &storage) != 0) {
        usage(argc, argv);
    }
    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)&storage;

    if (connect(s, addr, sizeof(storage)) != 0) {
        logexit("connect");
    }

    printf("Conectado ao servidor %s:%s\n", argv[1], argv[2]);

    struct action acao, resposta;
    while (1) {
        // Solicitar comando ao usuário
        memset(&acao, 0, sizeof(acao));
        printf("Digite um comando ('start', 'map', 'up', 'down', 'left', 'right', 'reset', 'quit'): ");
        char comando[16];
        scanf("%s", comando);

        // Preencher a estrutura `action` com base no comando
        if (strcmp(comando, "start") == 0) {
            acao.type = 0; // start
        } else if (strcmp(comando, "up") == 0) {
            acao.type = 1; // move
            acao.moves[0] = 1;
        } else if (strcmp(comando, "right") == 0) {
            acao.type = 1; // move
            acao.moves[0] = 2;
        } else if (strcmp(comando, "down") == 0) {
            acao.type = 1; // move
            acao.moves[0] = 3;
        } else if (strcmp(comando, "left") == 0) {
            acao.type = 1; // move
            acao.moves[0] = 4;
        } else if (strcmp(comando, "reset") == 0) {
            acao.type = 6; // reset
        } else if (strcmp(comando, "quit") == 0) {
            acao.type = 7; // exit
            send(s, &acao, sizeof(acao), 0);
            printf("Encerrando conexão...\n");
            break;
        } else if (strcmp(comando, "map") == 0) {
            acao.type = 2; // map
        } else {
            printf("error: command not found.\n");
            continue;
        }

        // Enviar comando para o servidor
        size_t count = send(s, &acao, sizeof(acao), 0);
        if (count != sizeof(acao)) {
            logexit("send");
        }

        // Receber resposta do servidor
        memset(&resposta, 0, sizeof(resposta));
        count = recv(s, &resposta, sizeof(resposta), 0);
        if (count == 0) {
            printf("Conexão encerrada pelo servidor.\n");
            break;
        } else if (count != sizeof(resposta)) {
            logexit("recv");
        }

        // Processar resposta do servidor
        if (resposta.type == -1) {
            printf("error: command not found\n");
        } else if (resposta.type == -2) {
            printf("error: start the game first\n");
        } else if (resposta.type == -3) {
            printf("error: you cannot go this way\n");
        } else if (acao.type == 2) { // Se o cliente enviou 'map'
            printf("Mapa completo recebido:\n");
            imprimir_labirinto(resposta.board);
        } else if (resposta.type == 4) { // Update padrão
            imprimir_movimentos(resposta.moves);
        } else if(resposta.type == 5) {
            printf("Parabéns! Você chegou na saída do labirinto!\n");
            //Imprimir o labirinto completo revelado
            imprimirLabirintoVitoria(resposta.board);
            break; // Opcional: encerrar o jogo após a vitória
        } else if(resposta.type == 6) {
            printf("Labirinto reiniciado!\n");
        } else {
            printf("Resposta inesperada do servidor.\n");
        }
    }

    close(s);
    return 0;
}