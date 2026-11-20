#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include "../include/http.h"
#include "../include/server.h"

#define QLEN 5
#define MAX_EVENTOS 64

int main()
{
    int passive_sock;
    char *service = "http";
    struct epoll_event ev, eventos[MAX_EVENTOS];

    passive_sock = passivesockTCP(service, QLEN);

    int fpoll = epoll_create1(0);
    struct sesion_sock smaster = {.fd_sock = passive_sock};
    ev.events = EPOLLIN;
    ev.data.ptr = &smaster;
    epoll_ctl(fpoll, EPOLL_CTL_ADD, passive_sock, &ev);

    while (1)
    {
        int fd_socklistos = epoll_wait(fpoll, eventos, MAX_EVENTOS, -1);
        for (int i = 0; i < fd_socklistos; i++)
        {
            struct sesion_sock *strueventos = (struct sesion_sock *)eventos[i].data.ptr;

            if (strueventos->fd_sock == passive_sock)
            {
                aceptarClientes(passive_sock, fpoll);
                continue;
            }

            // Error o desconexión inesperada
            if (eventos[i].events & (EPOLLERR | EPOLLHUP))
            {
                cerrarCliente(fpoll, strueventos);
                continue;
            }

            if ((eventos[i].events & EPOLLIN) && strueventos->estado == LEYENDO_PETICION)
            {
                manejarLectura(fpoll, strueventos);
            }

            if ((eventos[i].events & EPOLLOUT) && (strueventos->estado == ENVIANDO_CABECERAS || strueventos->estado == ENVIANDO_ARCHIVO))
            {
                manejarEscritura(fpoll, strueventos);
            }
        }
    }
}