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

// client.c hostname porta
int main(int argc, char *argv[]){
    int sd,err,nread;
    char input[16];
    char buffer[2048];
    struct addrinfo hints, *res, *ptr;
    
    if(argc!=3){
        fprintf(stderr,"Errore numero parametri\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1],argv[2],&hints,&res);
    if(err!=0){
        fprintf(stderr,"Errore getaddrinfo %s\n",gai_strerror(err));
        exit(EXIT_FAILURE);
    }
    for(ptr = res ; ptr != NULL; ptr=ptr->ai_next){
        sd = socket(ptr->ai_family,ptr->ai_socktype,ptr->ai_protocol);
        if(sd < 0){
            continue;
        }
        if(connect(sd,ptr->ai_addr,ptr->ai_addrlen) == 0){
            break;
        }
        close(sd);
    }

    freeaddrinfo(res);

    if (ptr == NULL) {
    fprintf(stderr, "Impossibile connettersi al server\n");
    exit(EXIT_FAILURE);
    }

    while(1){
        if((nread = read(sd,buffer,sizeof(buffer) -1)) <=0){
            break;
        }
        buffer[nread] = '\0';
        printf("%s",buffer);
        fflush(stdout);

        if(strstr(buffer, "giocatore") != NULL || strstr(buffer, "computer") != NULL || strstr(buffer, "pareggio") != NULL ){
            break;
        }

        if(fgets(input,sizeof(input),stdin) == NULL ) break;
        write_all(sd,input,strlen(input));
    }






    close(sd);
}