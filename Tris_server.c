#define POSIX_SOURCE 200809L
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

    for(;;){
        // Logica implementativa




    }
}