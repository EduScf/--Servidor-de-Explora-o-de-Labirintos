#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Bibliotecas de rede
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage(int argc, char **argv){
    print("usage: %s <server_ip> <server_port>\n");
    print("example: %s 127.0.01.1 51511\n");
    exit(EXIT_FAILURE);
}

void logexit(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    if(0 != connect(s, addr, sizeof(addr))){
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    printf("msg: ");
    fgets(buf, BUFSZ-1, stdin);
    int count = send(s, buf, strlen(buf)+1, 0);
    if(count != strlen(buf)+1){
        logexit("send");
    }

    memset(buf, 0, BUFSZ);
    unsigned total = 0;
    while(1){
        count = recv(s, buf, BUFSZ - total, 0);
        if(count == 0){
            //Conexão encerrada pelo servidor
            break;
        }
        if(count == -1){
            logexit("recv");
        }
        total += count;
        printf("received: %s\n", buf);
    }
    close(s);

    printf("received: %u bytes\n", total);
    puts(buf);

    exit(EXIT_SUCCESS);
}