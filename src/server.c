/* passivesock.c - passivesock */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

int passivesockTCP(char *service, int qlen)
{
    struct addrinfo hints, *result, *rep;
    int sfd = -1;
    int state;
    int opt = 1;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_socktype = SOCK_STREAM;

    state = getaddrinfo(NULL, service, &hints, &result);
    if (state != 0)
    {
        fprintf(stderr, "Fallo en getaddrinfo: %s\n", gai_strerror(state));
        exit(EXIT_FAILURE);
    }

    for (rep = result; rep != NULL; rep = rep->ai_next)
    {
        sfd = socket(rep->ai_family, rep->ai_socktype, rep->ai_protocol);
        if (sfd < 0)
        {
            continue;
        }

        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            perror("Error en setsockopt");
            close(sfd);
            exit(EXIT_FAILURE);
        }

        if (bind(sfd, rep->ai_addr, rep->ai_addrlen) == 0)
            break;

        close(sfd);
        sfd = -1;
    }

    freeaddrinfo(result);

    if (sfd < 0)
    {
        fprintf(stderr, "Fallo al hacer bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sfd, qlen) < 0)
    {
        close(sfd);
        fprintf(stderr, "Fallo al hacer listen\n");
        exit(EXIT_FAILURE);
    }

    return sfd;
}
