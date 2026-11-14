#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "utils.h"
#include <time.h>

void format_board(char board[3][3], char *buffer, size_t size){
            snprintf(buffer, size,
                "\n %c | %c | %c \n"
                "---+---+---\n"
                " %c | %c | %c \n"
                "---+---+---\n"
                " %c | %c | %c \n",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]);
        }

char check_winner(char b[3][3]){
    for(int i = 0; i < 3; i++){
        if(b[i][0] == b[i][1] && b[i][1] == b[i][2]){
            return b[i][0]; // Vincitore X || O 
        }
        if(b[0][i] == b[1][i] && b[1][i] == b[2][i]){
            return b[0][i]; // Vincitore X // O 
        }
    }
    if(b[0][0] == b[1][1] && b[1][1] == b[2][2]){
        return b[0][0];
    }
    if(b[0][2] == b[1][1] && b[1][1] == b[2][0]){
        return b[0][2];
    }
    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            if(b[i][j] != 'X' && b[i][j] != 'O'){
                return ' ';
            }
        }
    }
    return 'D';
}

void server_move(char m[3][3]) {
    int libere[9];
    int count = 0;

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (m[i][j] != 'X' && m[i][j] != 'O') {
                libere[count++] = i * 3 + j;
            }
        }
    }

    if (count > 0) {
        int scelta = libere[rand() % count];
        m[scelta / 3][scelta % 3] = 'O';
    }
}

int main(void){
    int sd, optval,err;
    struct addrinfo hints, *res;
    char buffer[2048];

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err = getaddrinfo(NULL,"5001",&hints,&res);
    if(err != 0){
        fprintf(stderr,"Errore getaddrinfo %s\n",gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    sd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
    if (sd < 0){
        perror("Errore creazione socket\n");
        exit(EXIT_FAILURE);
    }
    optval = 1;
    if(setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval)) < 0){
        perror("Errore setsockopt() \n");
        exit(EXIT_FAILURE);
    }

    if(bind(sd,res->ai_addr,res->ai_addrlen) < 0){
        perror("Errore bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(res);
    if(listen(sd,SOMAXCONN) < 0){
        perror("Errore listen \n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    for(;;){
        int ns, nread,move;
        char status;
        char board[3][3] = {{'1','2','3'},{'4','5','6'},{'7','8','9'}};
        struct sockaddr_storage client_address;

        socklen_t fromlen;

        fromlen = sizeof(client_address);
        ns= accept(sd, (struct sockaddr *)&client_address, &fromlen);
        if(ns < 0){
            perror("Errore in accept\n");
            exit(EXIT_FAILURE);
        }

        while(1){
        snprintf(buffer, sizeof(buffer),
            "\n %c | %c | %c \n"
            "---+---+---\n"
            " %c | %c | %c \n"
            "---+---+---\n"
            " %c | %c | %c \n"
            "\nTocca a te (1-9): ",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]
        );
        write_all(ns,buffer,strlen(buffer));
        if((nread = read(ns,buffer,sizeof(buffer) -1 )) <= 0 ){
            break;
        }
        buffer[nread] = '\0';
        if (sscanf(buffer, "%d", &move) != 1 || move < 1 || move > 9) continue;

        int r = (move - 1) / 3;
        int c = (move - 1) % 3;
        if (board[r][c] == 'X' || board[r][c] == 'O') continue;
        board[r][c] = 'X';

        status = check_winner(board);
        if (status != ' ') {
            format_board(board, buffer, sizeof(buffer));
            size_t len = strlen(buffer);
            if (status == 'X') {
                snprintf(buffer + len, sizeof(buffer) - len, "\nIl giocatore ha vinto\n");
            } else if (status == 'D') {
                snprintf(buffer + len, sizeof(buffer) - len, "\npareggio\n");
            }
            write_all(ns, buffer, strlen(buffer));
            break;
        }
        server_move(board);
        status = check_winner(board);

        if (status != ' ') {
            format_board(board, buffer, sizeof(buffer));
            size_t len = strlen(buffer);
            if (status == 'O') {
                snprintf(buffer + len, sizeof(buffer) - len, "\nIl computer ha vinto\n");
            } else if (status == 'D') {
                snprintf(buffer + len, sizeof(buffer) - len, "\npareggio\n");
            }
            write_all(ns, buffer, strlen(buffer));
            break;
        }
        
        }

        close(ns);

    }
}