//Coloque as bibliotecas padrões de C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUFSZ 1024

void usage(int argc, char **argv){
    printf("usage: %s <v4|v6> <server ports>\n", argv[0]);
    printf("example: %s v4 51511\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv){
    if(argc < 3){
        usage(argc, argv);
    }
    
    struct sockaddr_storage storage;
    if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr *addr = (struct sockaddr *) &storage;
    if(0 != bind(s, addr, sizeof(storage))){
        logexit("bind");
        exit(EXIT_FAILURE);
    }

    if(0 != listen(s, 10)){
        logexit("listen");
        exit(EXIT_FAILURE);
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connection\n", addrstr);

    while(1){
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) &cstorage;

        int r = accept(s, caddr, sizeof(cstorage));
        if(csock == -1){
            logexit("accept");
        }
        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count = recv(csock, buf, BUFSZ, 0);
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        sprintf(buf, "remore endpoint: %s\n", caddrstr);
        send(csock, buf, strlen(buf)+1, 0);
        close(csock);

    }

    exit(EXIT_SUCCESS);
}