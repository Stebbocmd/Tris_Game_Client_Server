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

int main(void)
{
    int sd, optval, err;
    char buff[2048];
    struct addrinfo hints, *res;


    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    //controllo errori 
    err=getaddrinfo(NULL, "5001", &hints, &res);

    if(err != 0)
    {
        fprintf(stderr, "errore getaddrinfo %s\n", gai_strerror(err));
        return EXIT_FAILURE;
    }

    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sd<0)
    {
        perror("errore creazione scoket\n");
        return EXIT_FAILURE;
    }

    optval = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval))<0)
    {
        perror("errore nel setsockopt\n");
        return EXIT_FAILURE;
    }

    if(bind(sd, res->ai_addr, res->ai_addrlen)<0)
    {
        perror("errore nel bind\n");
        return EXIT_FAILURE;

    }

    freeaddrinfo(res);
    if(listen(sd, SOMAXCONN)<0)
    {
        perror("errore nel listen\n");
        return EXIT_FAILURE;
    }
    for(;;)
    {
        int ns, nread;
        struct sockaddr_storage client_address;

        socklen_t fromlen;

        fromlen = sizeof(client_address);
        ns= accept(sd, (struct sockaddr *)&client_address, &fromlen);

        if(ns<0)
        {
            perror("errore in accept\n");
            return EXIT_FAILURE;
        }

        memset(buff, 0, sizeof(buff));

        nread = read(ns,buff,sizeof(buff)-1);
        if(nread < 0)
        {
            perror("errore in read\n ");
            close(ns);
            break;
        }
        char stringa1[sizeof(buff)] = {0};
        char stringa2[sizeof(buff)] = {0};
        sscanf(buff, "%s %s", stringa1, stringa2);
        
       
        
        if(strcmp(stringa1, stringa2)==0)
        {
            char risposta[4]= "SI\n";
            write(ns, risposta, sizeof(risposta));
        }
        else
        {
            char rispostaN[4]="NO\n";
            write(ns, rispostaN, sizeof(rispostaN));

        }

        close(ns);
    }

}