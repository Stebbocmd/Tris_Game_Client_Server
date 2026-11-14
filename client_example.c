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

int main(int argc, char *argv[])
{
    u_int8_t buff[2048];
    int sd, err, nread;
    struct addrinfo hints, *res, *ptr;
    char stringa1[20], stringa2[20];

    if(argc != 5)
    {
        fprintf(stderr, "errore argomenti uso hostname, porta, stringa1, stringa2\n");
        return EXIT_FAILURE;
    }
    strcpy(stringa1, argv[3]);

    strcpy(stringa2, argv[4]);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(argv[1], argv[2], &hints, &res);
    if(err!= 0)
    {
        fprintf(stderr, "errore getaddrinfp %s\n", gai_strerror(err));
        return EXIT_FAILURE;
    }
    
    for(ptr= res; ptr!= NULL; ptr=ptr->ai_next)
    {
        sd=socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sd<0)
        {
            continue;
        }
        if(connect(sd, ptr->ai_addr,ptr->ai_addrlen)==0)
        {
            break;
        }
        close(sd);
    }
    freeaddrinfo(res);

    char stringatot[50] = {0};
    strcat(stringatot,stringa1);
    strcat(stringatot, "\n");
    strcat(stringatot,stringa2);
    write(sd, stringatot, strlen(stringatot));
    
    char risposta[4] = {0};
    read(sd, &risposta, sizeof(risposta)-1);
    printf("%s", risposta);
    close(sd);
    

}